#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <functional>
#include <memory>
#include "../http/HttpRequest.hpp"
#include "../http/ResponseWriter.hpp"

namespace middleware {

class RateLimiter {
public:
    RateLimiter(size_t max_requests, std::chrono::seconds window);
    
    // The middleware handler
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

// Helper function to easily register middleware
inline auto rate_limit(size_t max_requests, std::chrono::seconds window) {
    return [limiter = std::make_shared<RateLimiter>(max_requests, window)](http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer) -> bool {
        return (*limiter)(req, writer);
    };
}

} // namespace middleware
