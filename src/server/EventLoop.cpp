#include "EventLoop.hpp"
#include <iostream>

namespace server {

EventLoop::EventLoop(Listener& listener, const routing::Router& router)
    : listener_(listener), connection_manager_(epoll_, router) {
    
    // Tell epoll to watch the listener socket for incoming connections (EPOLLIN)
    epoll_.add(listener_.fd(), EPOLLIN);
}

void EventLoop::run() {
    std::vector<epoll_event> events(64);

    std::cout << "Event loop started with ConnectionManager (HTTP Keep-Alive enabled)!\n";

    while (true) {
        // The thread SLEEPS here until the OS wakes it up. No more busy waiting!
        int num_ready = epoll_.wait(events);

        for (size_t i = 0; i < static_cast<size_t>(num_ready); ++i) {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            if (fd == listener_.fd()) {
                if (ev & EPOLLIN) {
                    handle_new_connections();
                }
            } else {
                if (ev & EPOLLIN) {
                    connection_manager_.handle_read(fd);
                }
                if (ev & EPOLLOUT) {
                    connection_manager_.handle_write(fd);
                }
            }
        }
    }
}

void EventLoop::handle_new_connections() {
    while (true) {
        auto client_opt = listener_.accept_connection();
        if (!client_opt) {
            break; // EAGAIN, we've accepted all currently waiting clients
        }

        network::Socket client = std::move(*client_opt);
        client.set_non_blocking();
        
        // Pass ownership of the new socket to the ConnectionManager
        connection_manager_.add_connection(std::move(client));
    }
}

} // namespace server
