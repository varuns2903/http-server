#include "EventLoop.hpp"
#include "../utils/Logger.hpp"
#include "../network/EpollProactor.hpp"
#include "../network/IoUringProactor.hpp"
#include <iostream>
#include <arpa/inet.h>

namespace server {

EventLoop::EventLoop(Listener& listener, const routing::Router& router, const config::ServerConfig& config, network::TlsContext* tls_context, network::UdpSocket* quic_socket, QuicConnectionManager* quic_manager)
    : listener_(listener), 
      proactor_(config.engine == config::EventEngine::Epoll ? 
               static_cast<std::unique_ptr<network::Proactor>>(std::make_unique<network::EpollProactor>()) : 
               static_cast<std::unique_ptr<network::Proactor>>(std::make_unique<network::IoUringProactor>())),
      thread_pool_(config.worker_threads), 
      connection_manager_(*proactor_, router, thread_pool_, timer_manager_, config.max_body_size, tls_context),
      quic_socket_(quic_socket),
      quic_manager_(quic_manager) {
    
    do_accept();

    if (quic_socket_ && quic_manager_) {
        do_read_quic();
    }
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

void EventLoop::do_read_quic() {
    proactor_->async_wait_read(quic_socket_->fd(), [this]() {
        char buffer[65536];
        sockaddr_in sender_addr;
        while (true) {
            ssize_t bytes_read = quic_socket_->recv_from(buffer, sizeof(buffer), sender_addr);
            if (bytes_read > 0) {
                quic_manager_->on_packet_received(reinterpret_cast<uint8_t*>(buffer), static_cast<size_t>(bytes_read), sender_addr);
            } else {
                break;
            }
        }
        if (is_running_) {
            do_read_quic();
        }
    });
}

} // namespace server
