#include <orbit/middleware/DistributedRateLimiter.hpp>
#include <orbit/http/HttpResponse.hpp>

namespace middleware {

DistributedRateLimiter::DistributedRateLimiter(const std::string& redis_host, int redis_port, size_t max_requests, std::chrono::seconds window)
    : max_requests_(max_requests), window_(window), redis_(std::make_unique<database::RedisClient>(redis_host, redis_port)) {}

bool DistributedRateLimiter::operator()(http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer) {
    std::string key = "rate:" + req.client_ip;
    
    long long count = redis_->incr(key);
    
    // If the key was just created, set its expiration
    if (count == 1) {
        redis_->expire(key, window_.count());
    }
    
    if (count > 0 && static_cast<size_t>(count) <= max_requests_) {
        return true;
    }
    
    // Rate limit exceeded or redis failed
    http::HttpResponse res;
    res.status(http::HttpStatus::TooManyRequests);
    res.set_body("429 Too Many Requests (Distributed limit exceeded)");
    res.headers["Connection"] = "close";
    writer->send(std::move(res));
    return false;
}

} // namespace middleware
