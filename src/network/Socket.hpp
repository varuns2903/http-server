#pragma once
#include <unistd.h>

namespace network {

class Socket {
public:
    // Create a new socket
    Socket();
    
    // Wrap an existing socket file descriptor
    explicit Socket(int fd);
    
    // Destructor: Closes the socket to prevent leaks (RAII)
    ~Socket();

    // Delete copy semantics to prevent double-closing
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    // Allow move semantics
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    int fd() const;
    bool is_valid() const;
    void close();
    
    // Convert socket to non-blocking mode
    void set_non_blocking();

private:
    int fd_;
};

} // namespace network
