#if defined(__APPLE__) || defined(__FreeBSD__)
#include "KqueueProactor.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>

namespace network {

KqueueProactor::KqueueProactor() : events_(64) {
    kq_fd_ = kqueue();
    if (kq_fd_ < 0) {
        throw std::runtime_error("Failed to create kqueue");
    }
}

KqueueProactor::~KqueueProactor() {
    if (kq_fd_ >= 0) close(kq_fd_);
}

void KqueueProactor::update_kqueue(Context& ctx) {
    std::vector<struct kevent> changes;
    
    // EVFILT_READ
    bool needs_read = ctx.reading || ctx.accepting || ctx.waiting_read;
    struct kevent ev_read;
    EV_SET(&ev_read, ctx.fd, EVFILT_READ, needs_read ? EV_ADD : EV_DELETE, 0, 0, nullptr);
    changes.push_back(ev_read);
    
    // EVFILT_WRITE
    bool needs_write = ctx.writing || ctx.connecting || ctx.waiting_write || ctx.sendfile_in_progress;
    struct kevent ev_write;
    EV_SET(&ev_write, ctx.fd, EVFILT_WRITE, needs_write ? EV_ADD : EV_DELETE, 0, 0, nullptr);
    changes.push_back(ev_write);

    kevent(kq_fd_, changes.data(), changes.size(), nullptr, 0, nullptr);
    ctx.tracked = true;
}

void KqueueProactor::remove(socket_t fd) {
    std::lock_guard<std::mutex> lock(ctx_mutex_);
    auto it = contexts_.find(fd);
    if (it != contexts_.end()) {
        struct kevent changes[2];
        EV_SET(&changes[0], fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
        EV_SET(&changes[1], fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
        kevent(kq_fd_, changes, 2, nullptr, 0, nullptr);
        contexts_.erase(it);
    }
}

void KqueueProactor::run_once(int timeout_ms) {
    struct timespec ts;
    ts.tv_sec = timeout_ms / 1000;
    ts.tv_nsec = (timeout_ms % 1000) * 1000000;
    
    int n = kevent(kq_fd_, nullptr, 0, events_.data(), events_.size(), timeout_ms >= 0 ? &ts : nullptr);
    if (n < 0) return;

    std::vector<struct kevent> ready_events(events_.begin(), events_.begin() + n);

    for (const auto& ev : ready_events) {
        handle_event(ev);
    }
}

void KqueueProactor::handle_event(const struct kevent& event) {
    int fd = event.ident;
    Context* ctx = nullptr;
    
    {
        std::lock_guard<std::mutex> lock(ctx_mutex_);
        auto it = contexts_.find(fd);
        if (it == contexts_.end()) return;
        ctx = &it->second;
    }

    if (event.flags & EV_EOF || event.flags & EV_ERROR) {
        // Handle EOF/Error gracefully via the read/write logic below
    }

    if (event.filter == EVFILT_READ) {
        if (ctx->accepting) {
            sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int new_fd = accept(fd, (struct sockaddr*)&client_addr, &client_len);
            ctx->accepting = false;
            update_kqueue(*ctx);
            if (ctx->accept_cb) ctx->accept_cb(new_fd, client_addr);
        } else if (ctx->reading) {
            ssize_t n = ::read(fd, ctx->read_buf, ctx->read_size);
            ctx->reading = false;
            update_kqueue(*ctx);
            if (ctx->read_cb) ctx->read_cb(n);
        } else if (ctx->waiting_read) {
            ctx->waiting_read = false;
            update_kqueue(*ctx);
            if (ctx->wait_read_cb) ctx->wait_read_cb();
        }
    }

    if (event.filter == EVFILT_WRITE) {
        if (ctx->connecting) {
            ctx->connecting = false;
            update_kqueue(*ctx);
            int err = 0;
            socklen_t len = sizeof(err);
            getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
            if (ctx->connect_cb) ctx->connect_cb(err);
        } else if (ctx->writing) {
            ssize_t n = ::write(fd, ctx->write_buf, ctx->write_size);
            ctx->writing = false;
            update_kqueue(*ctx);
            if (ctx->write_cb) ctx->write_cb(n);
        } else if (ctx->waiting_write) {
            ctx->waiting_write = false;
            update_kqueue(*ctx);
            if (ctx->wait_write_cb) ctx->wait_write_cb();
        } else if (ctx->sendfile_in_progress) {
            // macOS sendfile is different from Linux sendfile
            off_t len = ctx->sendfile_count;
            int ret = sendfile(ctx->sendfile_in_fd, fd, ctx->sendfile_offset, &len, nullptr, 0);
            
            if (ret == 0 || (ret == -1 && errno == EAGAIN && len > 0)) {
                ctx->sendfile_offset += len;
                ctx->sendfile_count -= len;
                if (ctx->sendfile_count == 0 || ret == 0) {
                    ctx->sendfile_in_progress = false;
                    update_kqueue(*ctx);
                    if (ctx->sendfile_cb) ctx->sendfile_cb(len);
                }
            } else {
                ctx->sendfile_in_progress = false;
                update_kqueue(*ctx);
                if (ctx->sendfile_cb) ctx->sendfile_cb(-1);
            }
        }
    }
}

// Setup methods (they mostly just set the state and call update_kqueue)
void KqueueProactor::async_read(socket_t fd, void* buffer, size_t size, std::function<void(ssize_t)> callback) {
    std::lock_guard<std::mutex> lock(ctx_mutex_);
    auto& ctx = contexts_[fd];
    ctx.fd = fd;
    ctx.reading = true;
    ctx.read_buf = buffer;
    ctx.read_size = size;
    ctx.read_cb = std::move(callback);
    update_kqueue(ctx);
}

void KqueueProactor::async_write(socket_t fd, const void* buffer, size_t size, std::function<void(ssize_t)> callback) {
    std::lock_guard<std::mutex> lock(ctx_mutex_);
    auto& ctx = contexts_[fd];
    ctx.fd = fd;
    ctx.writing = true;
    ctx.write_buf = buffer;
    ctx.write_size = size;
    ctx.write_cb = std::move(callback);
    update_kqueue(ctx);
}

void KqueueProactor::async_wait_read(socket_t fd, std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(ctx_mutex_);
    auto& ctx = contexts_[fd];
    ctx.fd = fd;
    ctx.waiting_read = true;
    ctx.wait_read_cb = std::move(callback);
    update_kqueue(ctx);
}

void KqueueProactor::async_wait_write(socket_t fd, std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(ctx_mutex_);
    auto& ctx = contexts_[fd];
    ctx.fd = fd;
    ctx.waiting_write = true;
    ctx.wait_write_cb = std::move(callback);
    update_kqueue(ctx);
}

void KqueueProactor::async_sendfile(socket_t out_fd, int in_fd, off_t offset, size_t count, std::function<void(ssize_t)> callback) {
    std::lock_guard<std::mutex> lock(ctx_mutex_);
    auto& ctx = contexts_[out_fd];
    ctx.fd = out_fd;
    ctx.sendfile_in_progress = true;
    ctx.sendfile_in_fd = in_fd;
    ctx.sendfile_offset = offset;
    ctx.sendfile_count = count;
    ctx.sendfile_cb = std::move(callback);
    update_kqueue(ctx);
}

void KqueueProactor::async_accept(socket_t fd, std::function<void(socket_t, sockaddr_in)> callback) {
    std::lock_guard<std::mutex> lock(ctx_mutex_);
    auto& ctx = contexts_[fd];
    ctx.fd = fd;
    ctx.accepting = true;
    ctx.accept_cb = std::move(callback);
    update_kqueue(ctx);
}

void KqueueProactor::async_connect(socket_t fd, const sockaddr_in& addr, std::function<void(int)> callback) {
    int ret = ::connect(fd, (const struct sockaddr*)&addr, sizeof(addr));
    if (ret == 0) {
        if (callback) callback(0);
        return;
    }
    if (errno != EINPROGRESS) {
        if (callback) callback(errno);
        return;
    }
    std::lock_guard<std::mutex> lock(ctx_mutex_);
    auto& ctx = contexts_[fd];
    ctx.fd = fd;
    ctx.connecting = true;
    ctx.connect_cb = std::move(callback);
    update_kqueue(ctx);
}

} // namespace network
#endif
