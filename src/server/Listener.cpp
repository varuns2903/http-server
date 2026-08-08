#include "Listener.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include "../utils/Logger.hpp"

namespace server {

Listener::Listener(uint16_t port) : port_(port), socket_() {
    // Enable SO_REUSEADDR
    int opt = 1;
    if (setsockopt(socket_.fd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        throw std::runtime_error(std::string("setsockopt SO_REUSEADDR failed: ") + std::strerror(errno));
    }
    
    // Set listening socket to non-blocking
    socket_.set_non_blocking();
}

void Listener::start() {
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);

    if (bind(socket_.fd(), reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
        throw std::runtime_error(std::string("bind failed: ") + std::strerror(errno));
    }

    if (listen(socket_.fd(), 10) == -1) {
        throw std::runtime_error(std::string("listen failed: ") + std::strerror(errno));
    }

    LOG_INFO("Listening on port " << port_ << "...");
}

std::optional<network::Socket> Listener::accept_connection() {
    sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);
    
    int client_fd = accept(socket_.fd(), reinterpret_cast<sockaddr*>(&client_addr), &client_addr_len);
    if (client_fd == -1) {
        // If the socket is non-blocking and no connections are present, accept returns EAGAIN or EWOULDBLOCK
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return std::nullopt;
        }
        throw std::runtime_error(std::string("accept failed: ") + std::strerror(errno));
    }

    LOG_DEBUG("Accepted connection from " << inet_ntoa(client_addr.sin_addr) 
              << ":" << ntohs(client_addr.sin_port));

    return network::Socket(client_fd);
}

int Listener::fd() const {
    return socket_.fd();
}

} // namespace server
