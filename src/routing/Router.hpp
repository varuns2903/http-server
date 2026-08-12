#pragma once
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../http/WebSocketConnection.hpp"
#include "../http/ResponseWriter.hpp"
#include "../openapi/OpenApi.hpp"
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <memory>

namespace routing {

using RouteHandler = std::function<void(http::HttpRequest&, std::shared_ptr<http::ResponseWriter>)>;
using Middleware = std::function<bool(http::HttpRequest&, std::shared_ptr<http::ResponseWriter>)>;
using WsHandler = std::function<void(http::websocket::WebSocketConnection&)>;
using ErrorHandler = std::function<void(const std::exception&, http::HttpRequest&, std::shared_ptr<http::ResponseWriter>)>;

struct DynamicRoute {
    http::HttpMethod method;
    std::vector<std::string> path_segments;
    RouteHandler handler;
};

class Router {
public:
    Router(const std::string& prefix = "", Router* parent = nullptr) 
        : prefix_(prefix), parent_(parent) {}

    class RouteBuilder {
    public:
        RouteBuilder(Router& router, http::HttpMethod method, const std::string& path, std::vector<Middleware> mws = {})
            : router_(router), method_(method), path_(path), mws_(std::move(mws)) {}
        
        RouteBuilder& summary(const std::string& s) { meta_.summary = s; return *this; }
        RouteBuilder& description(const std::string& s) { meta_.description = s; return *this; }
        RouteBuilder& tag(const std::string& t) { meta_.tags.push_back(t); return *this; }
        RouteBuilder& req_body(const std::string& schema_name) { meta_.request_body_schema = schema_name; return *this; }
        RouteBuilder& res_body(int status, const std::string& schema_name) { meta_.response_schemas[status] = schema_name; return *this; }
        
        void handler(RouteHandler h);
    private:
        Router& router_;
        http::HttpMethod method_;
        std::string path_;
        std::vector<Middleware> mws_;
        openapi::RouteMetadata meta_;
    };

    RouteBuilder route(const std::string& path, http::HttpMethod method = http::HttpMethod::GET) {
        return RouteBuilder(*this, method, path);
    }

    // Route Grouping
    void group(const std::string& prefix, std::function<void(Router&)> callback);

    // Register a handler for a specific HTTP method and path
    void add_route(http::HttpMethod method, const std::string& path, RouteHandler handler);
    void add_route(http::HttpMethod method, const std::string& path, std::vector<Middleware> mws, RouteHandler handler);
    void add_route_with_meta(http::HttpMethod method, const std::string& path, std::vector<Middleware> mws, const openapi::RouteMetadata& meta, RouteHandler handler);
    
    // Fluent routing API
    Router& get(const std::string& path, RouteHandler handler);
    Router& get(const std::string& path, std::vector<Middleware> mws, RouteHandler handler);
    
    Router& post(const std::string& path, RouteHandler handler);
    Router& post(const std::string& path, std::vector<Middleware> mws, RouteHandler handler);
    
    Router& put(const std::string& path, RouteHandler handler);
    Router& put(const std::string& path, std::vector<Middleware> mws, RouteHandler handler);
    
    Router& patch(const std::string& path, RouteHandler handler);
    Router& patch(const std::string& path, std::vector<Middleware> mws, RouteHandler handler);
    
    Router& del(const std::string& path, RouteHandler handler);
    Router& del(const std::string& path, std::vector<Middleware> mws, RouteHandler handler);
    
    Router& options(const std::string& path, RouteHandler handler);
    Router& options(const std::string& path, std::vector<Middleware> mws, RouteHandler handler);
    
    void ws(const std::string& path, WsHandler handler);
    void use(Middleware m);
    
    // Register global error handler
    void on_error(ErrorHandler handler);

    // Route an incoming request to the correct handler
    void route(http::HttpRequest& request, std::shared_ptr<http::ResponseWriter> response_writer) const;

    bool has_ws_route(const std::string& path) const;
    WsHandler get_ws_route(const std::string& path) const;

    // Check if a given path is registered as a streaming route
    bool is_stream_route(http::HttpMethod method, const std::string& path) const;
    void add_stream_route(http::HttpMethod method, const std::string& path, RouteHandler handler);

private:

    std::string make_route_key(http::HttpMethod method, std::string_view path) const;
    std::vector<std::string> split_path(std::string_view path) const;
    
    std::string prefix_;
    Router* parent_{nullptr};
    std::vector<Middleware> local_middlewares_;
    ErrorHandler error_handler_;
    
    std::unordered_map<std::string, RouteHandler> routes_;
    std::unordered_map<std::string, WsHandler> ws_routes_;
    std::vector<DynamicRoute> dynamic_routes_;
    std::vector<Middleware> middlewares_;
    
    std::unordered_set<std::string> stream_routes_;
    std::vector<DynamicRoute> dynamic_stream_routes_;
};

} // namespace routing
