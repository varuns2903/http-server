#include <orbit/server/Listener.hpp>
#include <orbit/network/PlatformSocket.hpp>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <orbit/utils/Logger.hpp>

namespace server {

Listener::Listener(uint16_t port) : port_(port), socket_() {
    // Enable SO_REUSEADDR
    int opt = 1;
#ifdef _WIN32
    if (setsockopt(socket_.fd(), SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) == network::SOCKET_ERROR_VAL) {
        throw std::runtime_error(std::string("setsockopt SO_REUSEADDR failed: ") + std::to_string(network::get_last_socket_error()));
    }
#else
    if (setsockopt(socket_.fd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == network::SOCKET_ERROR_VAL) {
        throw std::runtime_error(std::string("setsockopt SO_REUSEADDR failed: ") + std::to_string(network::get_last_socket_error()));
    }
    
    // Enable SO_REUSEPORT for zero-downtime hot reloading (POSIX only)
    if (setsockopt(socket_.fd(), SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == network::SOCKET_ERROR_VAL) {
        LOG_ERROR("setsockopt SO_REUSEPORT failed (Hot Reloading may not work): " << std::to_string(network::get_last_socket_error()));
    }
#endif
    
    // Set listening socket to non-blocking
    socket_.set_non_blocking();
}

void Listener::start() {
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);

    if (::bind(socket_.fd(), reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == network::SOCKET_ERROR_VAL) {
        throw std::runtime_error(std::string("bind failed: ") + std::to_string(network::get_last_socket_error()));
    }

    if (::listen(socket_.fd(), 10) == network::SOCKET_ERROR_VAL) {
        throw std::runtime_error(std::string("listen failed: ") + std::to_string(network::get_last_socket_error()));
    }

    LOG_INFO("Listening on port " << port_ << "...");
}

std::optional<network::Socket> Listener::accept_connection() {
    sockaddr_in client_addr{};
    network::socklen_t client_addr_len = sizeof(client_addr);
    
    network::socket_t client_fd = ::accept(socket_.fd(), reinterpret_cast<sockaddr*>(&client_addr), &client_addr_len);
    if (client_fd == network::INVALID_SOCKET_FD) {
        if (network::is_would_block_error()) {
            return std::nullopt;
        }
        throw std::runtime_error(std::string("accept failed: ") + std::to_string(network::get_last_socket_error()));
    }

    LOG_DEBUG("Accepted connection from " << inet_ntoa(client_addr.sin_addr) 
              << ":" << ntohs(client_addr.sin_port));

    return network::Socket(client_fd);
}

int Listener::fd() const {
    return socket_.fd();
}

} // namespace server
