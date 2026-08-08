#pragma once
#include "../network/Socket.hpp"
#include "../network/Epoll.hpp"
#include "../routing/Router.hpp"

namespace server {

class ConnectionManager; // Forward declaration

class Connection {
public:
    Connection(network::Socket socket, network::Epoll& epoll, const routing::Router& router, ConnectionManager& manager);
    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    void handle_read();
    void handle_write();

private:
    void send_data(std::string_view data);

    network::Socket socket_;
    network::Epoll& epoll_;
    const routing::Router& router_;
    ConnectionManager& manager_;
    
    // Persistent buffer for partial network reads
    std::vector<char> read_buffer_;
    // Persistent buffer for outbound responses
    std::vector<char> write_buffer_;

    // Helper to check if the buffer contains a full HTTP request
    bool is_request_complete() const;
    
    // Flag to track if the connection should be closed after writing finishes
    bool should_close_{false};
};

} // namespace server
