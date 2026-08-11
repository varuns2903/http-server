#pragma once
#include "Connection.hpp"
#include "../network/Proactor.hpp"
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
    ConnectionManager(network::Proactor& proactor, const routing::Router& router, concurrency::ThreadPool& thread_pool, TimerManager& timer_manager, size_t max_body_size, network::TlsContext* tls_context = nullptr);

    void add_connection(network::Socket socket, const std::string& client_ip);
    void remove_connection(int fd);
    
    size_t get_connection_count() const;

private:
    network::Proactor& proactor_;
    const routing::Router& router_;
    concurrency::ThreadPool& thread_pool_;
    TimerManager& timer_manager_;
    
    std::unordered_map<int, std::shared_ptr<Connection>> connections_;
    mutable std::mutex map_mutex_;
    size_t max_body_size_;
    network::TlsContext* tls_context_;
};

} // namespace server
