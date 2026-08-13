#pragma once
#include <orbit/network/Socket.hpp>
#include <cstdint>
#include <optional>

namespace server {

/**
 * @brief Listens for incoming network connections.
 */
class Listener {
public:
    /**
     * @brief Constructs a Listener on the specified port.
     * @param port The port to listen on.
     */
    explicit Listener(uint16_t port);
    
    /**
     * @brief Starts the listener to accept connections.
     */
    void start();

    /**
     * @brief Accepts a new incoming connection.
     * @return An optional Socket if a connection was accepted, std::nullopt otherwise.
     */
    std::optional<network::Socket> accept_connection();
    
    /**
     * @brief Gets the underlying file descriptor for epoll registration.
     * @return The file descriptor.
     */
    int fd() const;

private:
    uint16_t port_;
    network::Socket socket_;
};

} // namespace server
