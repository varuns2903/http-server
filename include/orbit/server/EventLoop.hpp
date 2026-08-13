#pragma once
#include <orbit/network/Proactor.hpp>
#include <orbit/server/Listener.hpp>
#include <orbit/routing/Router.hpp>
#include <orbit/server/ConnectionManager.hpp>
#include <orbit/concurrency/ThreadPool.hpp>
#include <orbit/server/TimerManager.hpp>
#include <orbit/config/Config.hpp>
#include <orbit/network/TlsContext.hpp>
#include <orbit/network/UdpSocket.hpp>
#include <orbit/server/QuicConnectionManager.hpp>
#include <atomic>

namespace server {

class EventLoop {
public:
    EventLoop(Listener& listener, const routing::Router& router, const config::ServerConfig& config, network::TlsContext* tls_context = nullptr, network::UdpSocket* quic_socket = nullptr, QuicConnectionManager* quic_manager = nullptr);
    void run();
    void stop();
    void stop_accepting();

    concurrency::ThreadPool& get_thread_pool() { return thread_pool_; }

private:
    void do_accept();
    void do_read_quic();

    Listener& listener_;
    std::unique_ptr<network::Proactor> proactor_;
    TimerManager timer_manager_;
    concurrency::ThreadPool thread_pool_;
    ConnectionManager connection_manager_;
    network::UdpSocket* quic_socket_;
    QuicConnectionManager* quic_manager_;
    
    
    std::atomic<bool> is_running_{true};
    std::atomic<bool> is_accepting_{true};
};

} // namespace server
