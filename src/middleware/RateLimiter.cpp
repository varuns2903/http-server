#include <orbit/middleware/RateLimiter.hpp>
#include <orbit/http/HttpResponse.hpp>

namespace middleware {

RateLimiter::RateLimiter(size_t max_requests, std::chrono::seconds window)
    : max_requests_(max_requests), window_(window) {}

bool RateLimiter::operator()(http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer) {
    bool allowed = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        auto& bucket = buckets_[req.client_ip];
        
        if (bucket.last_refill == std::chrono::steady_clock::time_point{}) {
            bucket.tokens = max_requests_;
            bucket.last_refill = now;
        } else {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - bucket.last_refill);
            if (elapsed >= window_) {
                bucket.tokens = max_requests_;
                bucket.last_refill = now;
            }
            if (elapsed > std::chrono::seconds(0)) {
                double rate = static_cast<double>(max_requests_) / static_cast<double>(window_.count());
                size_t new_tokens = static_cast<size_t>(static_cast<double>(elapsed.count()) * rate);
                if (new_tokens > 0) {
                    bucket.tokens = std::min(max_requests_, bucket.tokens + new_tokens);
                    bucket.last_refill = bucket.last_refill + std::chrono::seconds(static_cast<long long>(static_cast<double>(new_tokens) / rate));
                }
            }
        }
        
        if (bucket.tokens > 0) {
            bucket.tokens--;
            allowed = true;
        }
    }
    
    if (allowed) {
        return true;
    } else {
        http::HttpResponse res;
        res.status(http::HttpStatus::TooManyRequests);
        res.set_body("429 Too Many Requests");
        res.headers["Connection"] = "close";
        writer->send(std::move(res));
        return false;
    }
}

} // namespace middleware
