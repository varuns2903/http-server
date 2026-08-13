#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <functional>
#include <memory>
#include <orbit/http/HttpRequest.hpp>
#include <orbit/http/ResponseWriter.hpp>

namespace middleware {

/**
 * @ingroup middlewares
 * @brief Middleware for in-memory rate limiting.
 */
class RateLimiter {
public:
    /**
     * @brief Constructs a new Rate Limiter.
     * 
     * @param max_requests Maximum number of requests allowed in the time window.
     * @param window The time window for rate limiting.
     */
    RateLimiter(size_t max_requests, std::chrono::seconds window);
    
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
    struct Bucket {
        size_t tokens;
        std::chrono::steady_clock::time_point last_refill;
    };

    size_t max_requests_;
    std::chrono::seconds window_;
    std::unordered_map<std::string, Bucket> buckets_;
    std::mutex mutex_;
};

/**
 * @ingroup middlewares
 * @brief Helper function to easily register rate limiting middleware.
 * 
 * @param max_requests Maximum number of requests allowed in the time window.
 * @param window The time window for rate limiting.
 * @return A middleware handler function.
 */
inline auto rate_limit(size_t max_requests, std::chrono::seconds window) {
    return [limiter = std::make_shared<RateLimiter>(max_requests, window)](http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer) -> bool {
        return (*limiter)(req, writer);
    };
}

} // namespace middleware
