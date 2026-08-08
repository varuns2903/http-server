#pragma once
#include "../network/Epoll.hpp"
#include "Listener.hpp"
#include "../routing/Router.hpp"
#include "ConnectionManager.hpp"
#include "../concurrency/ThreadPool.hpp"
#include "TimerManager.hpp"
#include "../config/Config.hpp"
#include <atomic>

namespace server {

class EventLoop {
public:
    EventLoop(Listener& listener, const routing::Router& router, const config::ServerConfig& config);
    void run();
    void stop();

private:
    void handle_new_connections();

    Listener& listener_;
    network::Epoll epoll_;
    concurrency::ThreadPool thread_pool_;
    TimerManager timer_manager_;
    ConnectionManager connection_manager_;
    
    std::atomic<bool> is_running_{true};
};

} // namespace server
