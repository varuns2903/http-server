#pragma once
#include "Proactor.hpp"
#include <liburing.h>
#include <mutex>
#include <functional>

namespace network {

class IoUringProactor : public Proactor {
public:
    IoUringProactor(unsigned entries = 1024);
    ~IoUringProactor() override;

    void run_once(int timeout_ms) override;

    void async_read(int fd, void* buffer, size_t size, std::function<void(ssize_t)> callback) override;
    void async_write(int fd, const void* buffer, size_t size, std::function<void(ssize_t)> callback) override;
    void async_wait_read(int fd, std::function<void()> callback) override;
    void async_wait_write(int fd, std::function<void()> callback) override;
    void async_sendfile(int out_fd, int in_fd, off_t offset, size_t count, std::function<void(ssize_t)> callback) override;
    void async_accept(int fd, std::function<void(int, sockaddr_in)> callback) override;
    void async_connect(int fd, const sockaddr_in& addr, std::function<void(int)> callback) override;

    void remove(int fd) override;

private:
    struct io_uring ring_;
    std::mutex sq_mutex_;

    enum class OpType {
        READ,
        WRITE,
        WAIT_READ,
        WAIT_WRITE,
        SENDFILE,
        ACCEPT,
        CONNECT
    };

    struct IoContext {
        OpType type;
        int fd{-1};

        std::function<void(ssize_t)> io_cb;
        std::function<void()> wait_cb;
        std::function<void(int, sockaddr_in)> accept_cb;
        std::function<void(int)> connect_cb;
        
        sockaddr_in client_addr{};
        socklen_t client_len{sizeof(sockaddr_in)};

        // For sendfile fallback
        int in_fd{-1};
        off_t offset{0};
        size_t count{0};
    };

    struct io_uring_sqe* get_sqe_safe();
};

} // namespace network
