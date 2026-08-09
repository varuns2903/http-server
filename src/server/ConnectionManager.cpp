#include "ConnectionManager.hpp"
#include <iostream>

namespace server {

ConnectionManager::ConnectionManager(network::Epoll& epoll, const routing::Router& router, concurrency::ThreadPool& thread_pool, TimerManager& timer_manager, size_t max_body_size, network::TlsContext* tls_context)
    : epoll_(epoll), router_(router), thread_pool_(thread_pool), timer_manager_(timer_manager), max_body_size_(max_body_size), tls_context_(tls_context) {}

void ConnectionManager::add_connection(network::Socket socket) {
    int fd = socket.fd();
    // EPOLLONESHOT ensures epoll only triggers once per event, preventing concurrent thread access!
    epoll_.add(fd, EPOLLIN | EPOLLONESHOT);
    
    std::lock_guard<std::mutex> lock(map_mutex_);
    connections_[fd] = std::make_unique<Connection>(std::move(socket), epoll_, router_, *this, thread_pool_, timer_manager_, max_body_size_, tls_context_);
}

void ConnectionManager::remove_connection(int fd) {
    epoll_.remove(fd);
    
    std::unique_ptr<Connection> conn_to_destroy;
    {
        std::lock_guard<std::mutex> lock(map_mutex_);
        auto it = connections_.find(fd);
        if (it != connections_.end()) {
            conn_to_destroy = std::move(it->second);
            connections_.erase(it);
        }
    }
    // conn_to_destroy goes out of scope and is safely destructed outside map_mutex_!
}

void ConnectionManager::handle_read(int fd) {
    Connection* conn = nullptr;
    {
        std::lock_guard<std::mutex> lock(map_mutex_);
        auto it = connections_.find(fd);
        if (it != connections_.end()) conn = it->second.get();
    }
    if (conn) conn->handle_read();
}

void ConnectionManager::handle_write(int fd) {
    Connection* conn = nullptr;
    {
        std::lock_guard<std::mutex> lock(map_mutex_);
        auto it = connections_.find(fd);
        if (it != connections_.end()) conn = it->second.get();
    }
    if (conn) conn->handle_write();
}

} // namespace server
