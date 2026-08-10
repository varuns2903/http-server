#pragma once
#include <functional>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>

namespace network {

class Proactor {
public:
    virtual ~Proactor() = default;

    // Block and wait for completions
    virtual void run_once(int timeout_ms) = 0;

    // Asynchronous read
    virtual void async_read(int fd, void* buffer, size_t size, std::function<void(ssize_t bytes_read)> callback) = 0;

    // Asynchronous write
    virtual void async_write(int fd, const void* buffer, size_t size, std::function<void(ssize_t bytes_written)> callback) = 0;
    
    // Asynchronous wait for read readiness
    virtual void async_wait_read(int fd, std::function<void()> callback) = 0;

    // Asynchronous wait for write readiness
    virtual void async_wait_write(int fd, std::function<void()> callback) = 0;
    
    // Asynchronous sendfile
    virtual void async_sendfile(int out_fd, int in_fd, off_t offset, size_t count, std::function<void(ssize_t bytes_written)> callback) = 0;

    // Asynchronous accept
    virtual void async_accept(int fd, std::function<void(int new_fd, sockaddr_in addr)> callback) = 0;

    // Asynchronous connect
    virtual void async_connect(int fd, const sockaddr_in& addr, std::function<void(int status)> callback) = 0;

    // Cancel all operations and remove fd tracking
    virtual void remove(int fd) = 0;
};

} // namespace network
