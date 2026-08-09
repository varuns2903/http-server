#pragma once
#include <string>
#include <chrono>
#include <memory>
#include "../http/HttpRequest.hpp"
#include "../http/ResponseWriter.hpp"
#include "../database/RedisClient.hpp"

namespace middleware {

class DistributedRateLimiter {
public:
    DistributedRateLimiter(const std::string& redis_host, int redis_port, size_t max_requests, std::chrono::seconds window);
    
    bool operator()(http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer);

private:
    size_t max_requests_;
    std::chrono::seconds window_;
    std::unique_ptr<database::RedisClient> redis_;
};

inline auto distributed_rate_limit(const std::string& redis_host, int redis_port, size_t max_requests, std::chrono::seconds window) {
    return [limiter = std::make_shared<DistributedRateLimiter>(redis_host, redis_port, max_requests, window)]
           (http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer) -> bool {
        return (*limiter)(req, writer);
    };
}

} // namespace middleware
