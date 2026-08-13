#pragma once
#include <vector>
#include <queue>
#include <memory>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <orbit/network/Proactor.hpp>

namespace database {

template <typename ClientType>
class ConnectionPool : public std::enable_shared_from_this<ConnectionPool<ClientType>> {
public:
    using ClientFactory = std::function<std::shared_ptr<ClientType>()>;
    
    ConnectionPool(size_t max_size, ClientFactory factory)
        : max_size_(max_size), factory_(std::move(factory)) {}

    // Initialize the pool by establishing connections
    void init(std::function<void(std::shared_ptr<ClientType>, std::function<void(bool)>)> connector, std::function<void(bool success)> on_ready) {
        if (max_size_ == 0) {
            on_ready(true);
            return;
        }
        
        auto self = this->shared_from_this();
        auto success_count = std::make_shared<size_t>(0);
        auto fail_count = std::make_shared<size_t>(0);
        
        for (size_t i = 0; i < max_size_; ++i) {
            auto client = factory_();
            connector(client, [self, client, on_ready, success_count, fail_count](bool success) {
                std::lock_guard<std::mutex> lock(self->mutex_);
                if (success) {
                    self->idle_connections_.push(client);
                    (*success_count)++;
                } else {
                    (*fail_count)++;
                }
                
                if (*success_count + *fail_count == self->max_size_) {
                    on_ready(*fail_count == 0);
                }
            });
        }
    }

    // Acquire a connection asynchronously
    void acquire(std::function<void(std::shared_ptr<ClientType>)> callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!idle_connections_.empty()) {
            auto client = idle_connections_.front();
            idle_connections_.pop();
            callback(client);
        } else {
            // Queue the request
            wait_queue_.push(std::move(callback));
        }
    }

    // Release a connection back to the pool
    void release(std::shared_ptr<ClientType> client) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!wait_queue_.empty()) {
            auto next_callback = std::move(wait_queue_.front());
            wait_queue_.pop();
            // Dispatch immediately to next waiter
            next_callback(client);
        } else {
            idle_connections_.push(client);
        }
    }

private:
    size_t max_size_;
    ClientFactory factory_;
    
    std::mutex mutex_;
    std::queue<std::shared_ptr<ClientType>> idle_connections_;
    std::queue<std::function<void(std::shared_ptr<ClientType>)>> wait_queue_;
};

} // namespace database
