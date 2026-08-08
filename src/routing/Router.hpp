#pragma once
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include <functional>
#include <unordered_map>
#include <string>

namespace routing {

using RouteHandler = std::function<http::HttpResponse(const http::HttpRequest&)>;

class Router {
public:
    // Register a handler for a specific HTTP method and path
    void add_route(http::HttpMethod method, const std::string& path, RouteHandler handler);
    void set_static_dir(const std::string& dir);

    // Route an incoming request to the correct handler
    http::HttpResponse route(const http::HttpRequest& request) const;

private:
    std::string make_route_key(http::HttpMethod method, std::string_view path) const;
    std::unordered_map<std::string, RouteHandler> routes_;
    std::string static_dir_;
};

} // namespace routing
