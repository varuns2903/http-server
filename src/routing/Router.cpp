#include "Router.hpp"

namespace routing {

std::string Router::make_route_key(http::HttpMethod method, std::string_view path) const {
    std::string method_str;
    switch (method) {
        case http::HttpMethod::GET: method_str = "GET"; break;
        case http::HttpMethod::POST: method_str = "POST"; break;
        case http::HttpMethod::HEAD: method_str = "HEAD"; break;
        default: method_str = "UNKNOWN"; break;
    }
    return method_str + " " + std::string(path);
}

void Router::add_route(http::HttpMethod method, const std::string& path, RouteHandler handler) {
    std::string key = make_route_key(method, path);
    routes_[key] = std::move(handler);
}

http::HttpResponse Router::route(const http::HttpRequest& request) const {
    std::string key = make_route_key(request.method, request.uri);
    
    auto it = routes_.find(key);
    if (it != routes_.end()) {
        // We found a matching route, execute the handler!
        return it->second(request);
    }
    
    // If no route matches, return a 404 Not Found
    http::HttpResponse not_found;
    not_found.status_code = http::HttpStatus::NotFound;
    not_found.set_body("404 Not Found");
    return not_found;
}

} // namespace routing
