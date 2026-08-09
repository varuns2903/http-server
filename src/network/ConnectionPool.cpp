#include "ConnectionPool.hpp"
#include <unistd.h>
#include <sys/socket.h>

namespace network {

int ConnectionPool::acquire(const std::string& host, int port) {
    std::unique_lock<std::mutex> lock(mutex_);
    std::string key = host + ":" + std::to_string(port);
    
    auto it = pool_.find(key);
    if (it != pool_.end() && !it->second.empty()) {
        int fd = it->second.back().fd;
        it->second.pop_back();
        
        // Simple check if socket is still alive by peeking 1 byte without blocking
        char buf;
        ssize_t ret = recv(fd, &buf, 1, MSG_PEEK | MSG_DONTWAIT);
        if (ret == 0) {
            // Socket was closed by peer cleanly
            close(fd);
            // Recursive fallback to get the next one
            lock.unlock();
            return acquire(host, port);
        } else if (ret < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            // Socket has an error
            close(fd);
            lock.unlock();
            return acquire(host, port);
        }
        
        return fd;
    }
    
    return -1;
}

void ConnectionPool::release(const std::string& host, int port, int fd) {
    if (fd < 0) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = host + ":" + std::to_string(port);
    
    PooledConnection conn;
    conn.fd = fd;
    conn.last_used = std::chrono::steady_clock::now();
    
    pool_[key].push_back(conn);
}

void ConnectionPool::cleanup_stale_connections() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();
    
    for (auto& [key, conns] : pool_) {
        auto it = conns.begin();
        while (it != conns.end()) {
            if (std::chrono::duration_cast<std::chrono::seconds>(now - it->last_used).count() > 60) {
                close(it->fd);
                it = conns.erase(it);
            } else {
                ++it;
            }
        }
    }
}

} // namespace network
