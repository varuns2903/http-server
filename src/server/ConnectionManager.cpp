#include "ConnectionManager.hpp"
#include <iostream>

namespace server {

ConnectionManager::ConnectionManager(network::Epoll& epoll, const routing::Router& router)
    : epoll_(epoll), router_(router) {}

void ConnectionManager::add_connection(network::Socket socket) {
    int fd = socket.fd();
    epoll_.add(fd, EPOLLIN);
    connections_[fd] = std::make_unique<Connection>(std::move(socket), epoll_, router_, *this);
}

void ConnectionManager::remove_connection(int fd) {
    epoll_.remove(fd);
    connections_.erase(fd); // This safely destructs the Connection and closes the socket via RAII
}

void ConnectionManager::handle_read(int fd) {
    auto it = connections_.find(fd);
    if (it != connections_.end()) {
        it->second->handle_read();
    }
}

void ConnectionManager::handle_write(int fd) {
    auto it = connections_.find(fd);
    if (it != connections_.end()) {
        it->second->handle_write();
    }
}

} // namespace server
