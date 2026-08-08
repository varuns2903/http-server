#pragma once
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include <functional>
#include <unordered_map>
#include <vector>
#include <string>

namespace routing {

using RouteHandler = std::function<void(const http::HttpRequest&, http::HttpResponse&)>;
using Middleware = std::function<bool(http::HttpRequest&, http::HttpResponse&)>;

struct DynamicRoute {
    http::HttpMethod method;
    std::vector<std::string> path_segments;
    RouteHandler handler;
};

class Router {
public:
    // Register a handler for a specific HTTP method and path
    void add_route(http::HttpMethod method, const std::string& path, RouteHandler handler);
    void set_static_dir(const std::string& dir);
    void use(Middleware m);

    // Route an incoming request to the correct handler
    void route(http::HttpRequest& request, http::HttpResponse& response) const;

private:
    std::string make_route_key(http::HttpMethod method, std::string_view path) const;
    std::vector<std::string> split_path(std::string_view path) const;
    
    std::unordered_map<std::string, RouteHandler> routes_;
    std::vector<DynamicRoute> dynamic_routes_;
    std::vector<Middleware> middlewares_;
    std::string static_dir_;
};

} // namespace routing
