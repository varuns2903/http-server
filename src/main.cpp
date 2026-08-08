#include "server/Listener.hpp"
#include "server/EventLoop.hpp"
#include "routing/Router.hpp"
#include "config/Config.hpp"
#include "utils/Logger.hpp"
#include <iostream>
#include <csignal>

server::EventLoop* global_loop = nullptr;

void handle_signal(int signum) {
    LOG_INFO("Received signal " << signum << ". Initiating graceful shutdown...");
    if (global_loop) {
        global_loop->stop();
    }
}

int main(int argc, char* argv[]) {
    try {
        auto config = config::ServerConfig::parse(argc, argv);
        utils::Logger::init(config.log_level);

        std::signal(SIGINT, handle_signal);
        std::signal(SIGTERM, handle_signal);

        routing::Router router;
        
        router.add_route(http::HttpMethod::GET, "/", [](const http::HttpRequest& /*req*/) {
            http::HttpResponse res;
            res.set_body("<h1>Welcome to Phase 14: Config & Logging!</h1>", "text/html");
            return res;
        });

        router.add_route(http::HttpMethod::GET, "/api/data", [](const http::HttpRequest& /*req*/) {
            http::HttpResponse res;
            res.set_body("{\"status\": \"success\", \"message\": \"epoll is fast!\"}", "application/json");
            return res;
        });

        router.set_static_dir(config.static_dir);

        server::Listener listener(config.port);
        listener.start();

        server::EventLoop event_loop(listener, router, config);
        global_loop = &event_loop;
        
        event_loop.run();
        
        global_loop = nullptr;

    } catch (const std::exception& e) {
        LOG_ERROR("Fatal error: " << e.what());
        return 1;
    }

    LOG_INFO("Server shutdown complete. Goodbye!");
    return 0;
}
