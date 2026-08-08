#pragma once
#include "../network/Epoll.hpp"
#include "Listener.hpp"
#include "../routing/Router.hpp"
#include "ConnectionManager.hpp"

namespace server {

class EventLoop {
public:
    EventLoop(Listener& listener, const routing::Router& router);
    void run();

private:
    void handle_new_connections();

    Listener& listener_;
    network::Epoll epoll_;
    ConnectionManager connection_manager_;
};

} // namespace server
