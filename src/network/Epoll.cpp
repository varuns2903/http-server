#include "Epoll.hpp"
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <unistd.h>

namespace network {

Epoll::Epoll() {
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1) {
        throw std::runtime_error(std::string("epoll_create1 failed: ") + std::strerror(errno));
    }
}

Epoll::~Epoll() {
    if (epoll_fd_ != -1) {
        ::close(epoll_fd_);
    }
}

void Epoll::add(int fd, uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
        throw std::runtime_error(std::string("epoll_ctl ADD failed: ") + std::strerror(errno));
    }
}

void Epoll::modify(int fd, uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) == -1) {
        throw std::runtime_error(std::string("epoll_ctl MOD failed: ") + std::strerror(errno));
    }
}

void Epoll::remove(int fd) {
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        // Ignore errors on removal, the socket might already be closed by the OS.
    }
}

int Epoll::wait(std::vector<epoll_event>& events, int timeout_ms) {
    int num_events = epoll_wait(epoll_fd_, events.data(), static_cast<int>(events.size()), timeout_ms);
    if (num_events == -1 && errno != EINTR) {
        throw std::runtime_error(std::string("epoll_wait failed: ") + std::strerror(errno));
    }
    return num_events;
}

} // namespace network
