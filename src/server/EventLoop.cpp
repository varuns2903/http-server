#include "EventLoop.hpp"
#include "../utils/Logger.hpp"
#include "../network/EpollProactor.hpp"
#include "../network/IoUringProactor.hpp"
#include <iostream>
#include <arpa/inet.h>

namespace server {

EventLoop::EventLoop(Listener& listener, const routing::Router& router, const config::ServerConfig& config, network::TlsContext* tls_context)
    : listener_(listener), 
      proactor_(config.engine == config::EventEngine::Epoll ? 
               static_cast<std::unique_ptr<network::Proactor>>(std::make_unique<network::EpollProactor>()) : 
               static_cast<std::unique_ptr<network::Proactor>>(std::make_unique<network::IoUringProactor>())),
      thread_pool_(config.worker_threads), 
      connection_manager_(*proactor_, router, thread_pool_, timer_manager_, config.max_body_size, tls_context) {
    
    do_accept();
}

void EventLoop::run() {
    LOG_INFO("Event loop started with ConnectionManager (HTTP Keep-Alive enabled)!");

    while (is_running_) {
        try {
            proactor_->run_once(timer_manager_.get_next_timeout());

            timer_manager_.handle_expired_timers([this](int fd) {
                connection_manager_.remove_connection(fd);
            });
        } catch (const std::exception& e) {
            LOG_ERROR("Error in event loop: " + std::string(e.what()));
        }
    }
    
    LOG_INFO("Event loop stopped. Shutting down...");
}

void EventLoop::stop() {
    is_running_ = false;
}

void EventLoop::do_accept() {
    proactor_->async_accept(listener_.fd(), [this](int client_fd, sockaddr_in addr) {
        if (client_fd >= 0) {
            std::cout << "Accepted new connection! FD: " << client_fd << std::endl;
            std::string client_ip = inet_ntoa(addr.sin_addr);
            network::Socket client(client_fd);
            connection_manager_.add_connection(std::move(client), client_ip);
        } else {
            LOG_ERROR("Accept failed");
        }
        
        if (is_running_) {
            do_accept();
        }
    });
}

} // namespace server
