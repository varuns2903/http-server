#pragma once
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../http/WebSocketConnection.hpp"
#include "../http/ResponseWriter.hpp"
#include <functional>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

namespace routing {

using RouteHandler = std::function<void(http::HttpRequest&, std::shared_ptr<http::ResponseWriter>)>;
using Middleware = std::function<bool(http::HttpRequest&, std::shared_ptr<http::ResponseWriter>)>;
using WsHandler = std::function<void(http::websocket::WebSocketConnection&)>;

struct DynamicRoute {
    http::HttpMethod method;
    std::vector<std::string> path_segments;
    RouteHandler handler;
};

class Router {
public:
    Router() = default;

    // Route Grouping
    void group(const std::string& prefix, std::function<void(Router&)> callback);

    // Register a handler for a specific HTTP method and path
    void add_route(http::HttpMethod method, const std::string& path, RouteHandler handler);
    void ws(const std::string& path, WsHandler handler);
    void use(Middleware m);

    // Route an incoming request to the correct handler
    void route(http::HttpRequest& request, std::shared_ptr<http::ResponseWriter> response_writer) const;

    bool has_ws_route(const std::string& path) const;
    WsHandler get_ws_route(const std::string& path) const;

private:
    Router(const std::string& prefix, Router* parent) : prefix_(prefix), parent_(parent) {}

    std::string make_route_key(http::HttpMethod method, std::string_view path) const;
    std::vector<std::string> split_path(std::string_view path) const;
    
    std::string prefix_;
    Router* parent_{nullptr};
    std::vector<Middleware> local_middlewares_;
    
    std::unordered_map<std::string, RouteHandler> routes_;
    std::unordered_map<std::string, WsHandler> ws_routes_;
    std::vector<DynamicRoute> dynamic_routes_;
    std::vector<Middleware> middlewares_;
};

} // namespace routing
