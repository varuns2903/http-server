#pragma once
#include "../network/Proactor.hpp"
#include "Listener.hpp"
#include "../routing/Router.hpp"
#include "ConnectionManager.hpp"
#include "../concurrency/ThreadPool.hpp"
#include "TimerManager.hpp"
#include "../config/Config.hpp"
#include "../network/TlsContext.hpp"
#include <atomic>

namespace server {

class EventLoop {
public:
    EventLoop(Listener& listener, const routing::Router& router, const config::ServerConfig& config, network::TlsContext* tls_context = nullptr);
    void run();
    void stop();

private:
    void do_accept();

    Listener& listener_;
    std::unique_ptr<network::Proactor> proactor_;
    concurrency::ThreadPool thread_pool_;
    TimerManager timer_manager_;
    ConnectionManager connection_manager_;
    
    std::atomic<bool> is_running_{true};
};

} // namespace server
