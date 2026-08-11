#pragma once

#include "Listener.hpp"
#include "EventLoop.hpp"
#include "../routing/Router.hpp"
#include "../config/Config.hpp"
#include "../network/TlsContext.hpp"
#include "../network/UdpSocket.hpp"
#include "QuicConnectionManager.hpp"
#include <memory>

namespace server {

class App {
public:
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

    // Metrics
    App& enable_metrics(const std::string& path = "/metrics");

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
