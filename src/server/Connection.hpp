#pragma once
#include "../network/Socket.hpp"
#include "../network/Epoll.hpp"
#include "../routing/Router.hpp"
#include "../concurrency/ThreadPool.hpp"
#include "TimerManager.hpp"
#include <vector>
#include <string_view>

namespace server {

class ConnectionManager; // Forward declaration

class Connection {
public:
    Connection(network::Socket socket, network::Epoll& epoll, const routing::Router& router, ConnectionManager& manager, concurrency::ThreadPool& thread_pool, TimerManager& timer_manager);
    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    void handle_read();
    void handle_write();

private:
    void process_request();
    void send_data(std::string_view data);
    void reset_timer();

    network::Socket socket_;
    network::Epoll& epoll_;
    const routing::Router& router_;
    ConnectionManager& manager_;
    concurrency::ThreadPool& thread_pool_;
    TimerManager& timer_manager_;
    
    std::vector<char> read_buffer_;
    std::vector<char> write_buffer_;

    bool is_request_complete() const;
    bool should_close_{false};
    uint64_t current_timer_id_{0};
    
    // For zero-copy sendfile
    int file_fd_{-1};
    off_t file_size_{0};
    off_t file_offset_{0};
};

} // namespace server
