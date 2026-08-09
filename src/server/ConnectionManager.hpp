#pragma once
#include "Connection.hpp"
#include "../network/Epoll.hpp"
#include "../routing/Router.hpp"
#include "../concurrency/ThreadPool.hpp"
#include "TimerManager.hpp"
#include "../network/TlsContext.hpp"
#include <unordered_map>
#include <memory>
#include <mutex>

namespace server {

class ConnectionManager {
public:
    ConnectionManager(network::Epoll& epoll, const routing::Router& router, concurrency::ThreadPool& thread_pool, TimerManager& timer_manager, size_t max_body_size, network::TlsContext* tls_context = nullptr);

    void add_connection(network::Socket socket);
    void remove_connection(int fd);
    void handle_read(int fd);
    void handle_write(int fd);

private:
    network::Epoll& epoll_;
    const routing::Router& router_;
    concurrency::ThreadPool& thread_pool_;
    TimerManager& timer_manager_;
    
    std::unordered_map<int, std::unique_ptr<Connection>> connections_;
    std::mutex map_mutex_;
    size_t max_body_size_;
    network::TlsContext* tls_context_;
};

} // namespace server
