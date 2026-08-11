#include "Metrics.hpp"
#include "../utils/PrometheusRegistry.hpp"
#include <chrono>

namespace middleware {

routing::Middleware Metrics::track() {
    return [](http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> res_writer) -> bool {
        auto start = std::chrono::high_resolution_clock::now();
        
        // We use a shared_ptr to track when the response writer goes out of scope (or when it completes)
        // Alternatively, we just track the routing time synchronously here:
        
        // Just before this middleware returns, we don't know the status code.
        // We can inject a proxy ResponseWriter to intercept the status code, or simply track it as a generic request.
        
        std::string method_str = "UNKNOWN";
        switch (req.method) {
            case http::HttpMethod::GET: method_str = "GET"; break;
            case http::HttpMethod::POST: method_str = "POST"; break;
            case http::HttpMethod::PUT: method_str = "PUT"; break;
            case http::HttpMethod::PATCH: method_str = "PATCH"; break;
            case http::HttpMethod::DELETE: method_str = "DELETE"; break;
            case http::HttpMethod::OPTIONS: method_str = "OPTIONS"; break;
            case http::HttpMethod::HEAD: method_str = "HEAD"; break;
            default: break;
        }

        utils::PrometheusRegistry::get_instance().inc_counter(
            "orbit_http_requests_total", 
            "method=\"" + method_str + "\""
        );
        
        return true; 
    };
}

} // namespace middleware
