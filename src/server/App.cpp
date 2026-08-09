#include "App.hpp"
#include "../utils/Logger.hpp"

namespace server {

App::App(const config::ServerConfig& config) : config_(config) {
    utils::Logger::init(config.log_level);
    if (!config_.ssl_cert.empty() && !config_.ssl_key.empty()) {
        tls_context_ = std::make_unique<network::TlsContext>(config_.ssl_cert, config_.ssl_key);
    }
}

App::~App() {
    stop();
}

App& App::use(routing::Middleware m) {
    router_.use(std::move(m));
    return *this;
}

App& App::get(const std::string& path, routing::RouteHandler handler) {
    router_.add_route(http::HttpMethod::GET, path, std::move(handler));
    return *this;
}

App& App::post(const std::string& path, routing::RouteHandler handler) {
    router_.add_route(http::HttpMethod::POST, path, std::move(handler));
    return *this;
}

App& App::put(const std::string& path, routing::RouteHandler handler) {
    router_.add_route(http::HttpMethod::PUT, path, std::move(handler));
    return *this;
}

App& App::patch(const std::string& path, routing::RouteHandler handler) {
    router_.add_route(http::HttpMethod::PATCH, path, std::move(handler));
    return *this;
}

App& App::del(const std::string& path, routing::RouteHandler handler) {
    router_.add_route(http::HttpMethod::DELETE, path, std::move(handler));
    return *this;
}

App& App::options(const std::string& path, routing::RouteHandler handler) {
    router_.add_route(http::HttpMethod::OPTIONS, path, std::move(handler));
    return *this;
}

void App::listen() {
    listener_ = std::make_unique<Listener>(config_.port);
    listener_->start();
    
    event_loop_ = std::make_unique<EventLoop>(*listener_, router_, config_, tls_context_.get());
    
    LOG_INFO("App started listening on port " << config_.port);
    event_loop_->run();
}

void App::stop() {
    if (event_loop_) {
        event_loop_->stop();
    }
}

} // namespace server
