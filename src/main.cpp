#include "server/Listener.hpp"
#include "server/EventLoop.hpp"
#include "routing/Router.hpp"
#include <iostream>
#include <csignal>

constexpr int PORT = 8080;

server::EventLoop* global_loop = nullptr;

void handle_signal(int signum) {
    std::cout << "\nReceived signal " << signum << ". Initiating graceful shutdown...\n";
    if (global_loop) {
        global_loop->stop();
    }
}

int main() {
    try {
        std::signal(SIGINT, handle_signal);
        std::signal(SIGTERM, handle_signal);

        routing::Router router;
        
        router.add_route(http::HttpMethod::GET, "/", [](const http::HttpRequest& /*req*/) {
            http::HttpResponse res;
            res.set_body("<h1>Welcome to Phase 12: Graceful Shutdown!</h1>", "text/html");
            return res;
        });

        router.add_route(http::HttpMethod::GET, "/api/data", [](const http::HttpRequest& /*req*/) {
            http::HttpResponse res;
            res.set_body("{\"status\": \"success\", \"message\": \"epoll is fast!\"}", "application/json");
            return res;
        });

        router.add_route(http::HttpMethod::GET, "/test", [](const http::HttpRequest& /*req*/) {
            http::HttpResponse res;
            res.send_file("test.txt", "text/plain");
            return res;
        });

        server::Listener listener(PORT);
        listener.start();

        server::EventLoop event_loop(listener, router);
        global_loop = &event_loop;
        
        event_loop.run();
        
        global_loop = nullptr;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "Server shutdown complete. Goodbye!\n";
    return 0;
}
