#pragma once
#include "../network/Socket.hpp"
#include <cstdint>
#include <optional>

namespace server {

class Listener {
public:
    explicit Listener(uint16_t port);
    
    void start();
    std::optional<network::Socket> accept_connection();
    
    // Get the underlying file descriptor for epoll registration
    int fd() const;

private:
    uint16_t port_;
    network::Socket socket_;
};

} // namespace server
