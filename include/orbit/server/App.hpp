#pragma once

#include <orbit/server/Listener.hpp>
#include <orbit/server/EventLoop.hpp>
#include <orbit/routing/Router.hpp>
#include <orbit/config/Config.hpp>
#include <orbit/network/TlsContext.hpp>
#include <orbit/network/UdpSocket.hpp>
#include <orbit/server/QuicConnectionManager.hpp>
#include <memory>

namespace server {

/**
 * @brief The main application class for the Orbit Framework.
 * 
 * The App class acts as the central orchestrator for the web framework. It manages the server's lifecycle,
 * routing, middlewares, dependency injection, and worker thread pools. 
 * Users instantiate this class, define their routes, and call `listen()` to start accepting connections.
 */
class App {
public:
    /**
     * @brief Constructs a new App instance.
     * @param config The server configuration containing port, worker threads, and event engine preferences.
     */
    App(const config::ServerConfig& config);
    ~App();

    // Delete copy constructors
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    // Middleware
    void on_error(routing::ErrorHandler handler);
    App& use(routing::Middleware m);

    // Route Grouping
    App& group(const std::string& prefix, std::function<void(routing::Router&)> callback) {
        router_.group(prefix, std::move(callback));
        return *this;
    }

    routing::Router::RouteBuilder route(const std::string& path, http::HttpMethod method = http::HttpMethod::GET) {
        return router_.route(path, method);
    }

    // Fluent routing API
    App& get(const std::string& path, routing::RouteHandler handler);
    App& get(const std::string& path, std::vector<routing::Middleware> mws, routing::RouteHandler handler);
    
    App& post(const std::string& path, routing::RouteHandler handler);
    App& post(const std::string& path, std::vector<routing::Middleware> mws, routing::RouteHandler handler);
    
    App& put(const std::string& path, routing::RouteHandler handler);
    App& put(const std::string& path, std::vector<routing::Middleware> mws, routing::RouteHandler handler);
    
    App& patch(const std::string& path, routing::RouteHandler handler);
    App& patch(const std::string& path, std::vector<routing::Middleware> mws, routing::RouteHandler handler);
    
    App& del(const std::string& path, routing::RouteHandler handler);
    App& del(const std::string& path, std::vector<routing::Middleware> mws, routing::RouteHandler handler);
    
    App& options(const std::string& path, routing::RouteHandler handler);
    App& options(const std::string& path, std::vector<routing::Middleware> mws, routing::RouteHandler handler);
    
    // WebSockets
    App& ws(const std::string& path, routing::WsHandler handler) {
        router_.ws(path, std::move(handler));
        return *this;
    }

    concurrency::ThreadPool& get_thread_pool() {
        if (!event_loop_) throw std::runtime_error("Server not started");
        return event_loop_->get_thread_pool();
    }

    // Metrics
    App& enable_metrics(const std::string& path = "/metrics");

    // OpenAPI & Swagger UI
    App& enable_openapi(const std::string& title = "Orbit Framework API", 
                        const std::string& version = "1.0.0", 
                        const std::string& docs_path = "/docs", 
                        const std::string& json_path = "/swagger.json");

    // Start the server (blocking)
    void listen();
    
    // Stop the server gracefully
    void stop();
    
    // Hot reload the server
    void hot_reload();

private:
    config::ServerConfig config_;
    routing::Router router_;
    std::unique_ptr<Listener> listener_;
    std::unique_ptr<network::UdpSocket> quic_socket_;
    std::unique_ptr<QuicConnectionManager> quic_manager_;
    std::unique_ptr<network::TlsContext> tls_context_;
    std::unique_ptr<EventLoop> event_loop_;
};

} // namespace server
