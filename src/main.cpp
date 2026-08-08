#include "server/Listener.hpp"
#include "server/EventLoop.hpp"
#include "routing/Router.hpp"
#include <iostream>

constexpr int PORT = 8080;

int main() {
    try {
        routing::Router router;
        
        router.add_route(http::HttpMethod::GET, "/", [](const http::HttpRequest& /*req*/) {
            http::HttpResponse res;
            res.set_body("<h1>Welcome to Phase 5: The Epoll Event Loop!</h1>", "text/html");
            return res;
        });

        router.add_route(http::HttpMethod::GET, "/api/data", [](const http::HttpRequest& /*req*/) {
            http::HttpResponse res;
            res.set_body("{\"status\": \"success\", \"message\": \"epoll is fast!\"}", "application/json");
            return res;
        });

        server::Listener listener(PORT);
        listener.start();

        // The EventLoop now completely takes over execution!
        server::EventLoop event_loop(listener, router);
        event_loop.run();

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
