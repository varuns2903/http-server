#pragma once

#include "Listener.hpp"
#include "EventLoop.hpp"
#include "../routing/Router.hpp"
#include "../config/Config.hpp"
#include "../network/TlsContext.hpp"
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
    App& use(routing::Middleware m);

    // Fluent routing API
    App& get(const std::string& path, routing::RouteHandler handler);
    App& post(const std::string& path, routing::RouteHandler handler);
    App& put(const std::string& path, routing::RouteHandler handler);
    App& patch(const std::string& path, routing::RouteHandler handler);
    App& del(const std::string& path, routing::RouteHandler handler);
    App& options(const std::string& path, routing::RouteHandler handler);
    
    // WebSockets
    App& ws(const std::string& path, routing::WsHandler handler) {
        router_.ws(path, std::move(handler));
        return *this;
    }

    // Start the server (blocking)
    void listen();
    
    // Stop the server gracefully
    void stop();

private:
    config::ServerConfig config_;
    routing::Router router_;
    std::unique_ptr<Listener> listener_;
    std::unique_ptr<network::TlsContext> tls_context_;
    std::unique_ptr<EventLoop> event_loop_;
};

} // namespace server
