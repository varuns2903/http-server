#include "Socket.hpp"
#include <sys/socket.h>
#include <stdexcept>
#include <string>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

namespace network {

Socket::Socket() {
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ == -1) {
        throw std::runtime_error(std::string("Failed to create socket: ") + std::strerror(errno));
    }
}

Socket::Socket(int fd) : fd_(fd) {}

Socket::~Socket() {
    close();
}

Socket::Socket(Socket&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        close();
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

int Socket::fd() const {
    return fd_;
}

bool Socket::is_valid() const {
    return fd_ != -1;
}

void Socket::close() {
    if (fd_ != -1) {
        ::close(fd_);
        fd_ = -1;
    }
}

void Socket::set_non_blocking() {
    if (fd_ == -1) return;

    int flags = fcntl(fd_, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error(std::string("fcntl F_GETFL failed: ") + std::strerror(errno));
    }

    if (fcntl(fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error(std::string("fcntl F_SETFL O_NONBLOCK failed: ") + std::strerror(errno));
    }
}

} // namespace network
