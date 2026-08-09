#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <chrono>

namespace network {

struct PooledConnection {
    int fd{-1};
    std::chrono::steady_clock::time_point last_used;
};

class ConnectionPool {
public:
    static ConnectionPool& get_instance() {
        static ConnectionPool instance;
        return instance;
    }

    // Gets a connection immediately if available, or -1 if none idle
    int acquire(const std::string& host, int port);
    
    // Returns a connection back to the pool
    void release(const std::string& host, int port, int fd);
    
    // Removes closed or stale connections
    void cleanup_stale_connections();

private:
    ConnectionPool() = default;
    
    std::mutex mutex_;
    // Key: "host:port"
    std::unordered_map<std::string, std::vector<PooledConnection>> pool_;
};

} // namespace network
