#pragma once
#include "Connection.hpp"
#include "../network/Epoll.hpp"
#include "../routing/Router.hpp"
#include <unordered_map>
#include <memory>

namespace server {

class ConnectionManager {
public:
    ConnectionManager(network::Epoll& epoll, const routing::Router& router);

    void add_connection(network::Socket socket);
    void remove_connection(int fd);
    void handle_read(int fd);
    void handle_write(int fd);

private:
    network::Epoll& epoll_;
    const routing::Router& router_;
    std::unordered_map<int, std::unique_ptr<Connection>> connections_;
};

} // namespace server
