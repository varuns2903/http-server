#pragma once
#include <string>
#include <chrono>
#include <memory>
#include <orbit/http/HttpRequest.hpp>
#include <orbit/http/ResponseWriter.hpp>
#include <orbit/database/RedisClient.hpp>

namespace middleware {

/**
 * @ingroup middlewares
 * @brief Distributed rate limiter middleware using Redis.
 */
class DistributedRateLimiter {
public:
    /**
     * @brief Constructs a new Distributed Rate Limiter.
     * 
     * @param redis_host The Redis server host.
     * @param redis_port The Redis server port.
     * @param max_requests Maximum number of requests allowed in the time window.
     * @param window The time window for rate limiting.
     */
    DistributedRateLimiter(const std::string& redis_host, int redis_port, size_t max_requests, std::chrono::seconds window);
    
    /**
     * @brief Middleware execution operator.
     * 
     * @param req The HTTP request.
     * @param writer The HTTP response writer.
     * @return true If the request is allowed.
     * @return false If the request is rate-limited.
     */
    bool operator()(http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer);

private:
    size_t max_requests_;
    std::chrono::seconds window_;
    std::unique_ptr<database::RedisClient> redis_;
};

/**
 * @ingroup middlewares
 * @brief Helper to create a distributed rate limiting middleware handler.
 * 
 * @param redis_host The Redis server host.
 * @param redis_port The Redis server port.
 * @param max_requests Maximum number of requests allowed in the time window.
 * @param window The time window for rate limiting.
 * @return A middleware handler function.
 */
inline auto distributed_rate_limit(const std::string& redis_host, int redis_port, size_t max_requests, std::chrono::seconds window) {
    return [limiter = std::make_shared<DistributedRateLimiter>(redis_host, redis_port, max_requests, window)]
           (http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer) -> bool {
        return (*limiter)(req, writer);
    };
}

} // namespace middleware
