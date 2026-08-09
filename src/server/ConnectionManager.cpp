#include "ConnectionManager.hpp"
#include <iostream>

namespace server {

ConnectionManager::ConnectionManager(network::Proactor& proactor, const routing::Router& router, concurrency::ThreadPool& thread_pool, TimerManager& timer_manager, size_t max_body_size, network::TlsContext* tls_context)
    : proactor_(proactor), router_(router), thread_pool_(thread_pool), timer_manager_(timer_manager), max_body_size_(max_body_size), tls_context_(tls_context) {}

void ConnectionManager::add_connection(network::Socket socket, const std::string& client_ip) {
    int fd = socket.fd();
    auto connection = std::make_shared<Connection>(
        std::move(socket), client_ip, proactor_, router_, *this, thread_pool_, timer_manager_, max_body_size_, tls_context_
    );
    
    {
        std::lock_guard<std::mutex> lock(map_mutex_);
        connections_[fd] = connection;
    }
    
    // With Proactor, we kick off the first read immediately!
    connection->start();
}

void ConnectionManager::remove_connection(int fd) {
    std::shared_ptr<Connection> conn;
    {
        std::lock_guard<std::mutex> lock(map_mutex_);
        auto it = connections_.find(fd);
        if (it != connections_.end()) {
            conn = it->second;
            connections_.erase(it);
        }
    }
    
    if (conn) {
        // Cancel all pending asynchronous operations in the Proactor
        proactor_.remove(fd);
        // The shared_ptr will be destroyed here, triggering Connection::~Connection
    }
}

} // namespace server
