#include "server/App.hpp"
#include "config/Config.hpp"
#include "utils/Logger.hpp"
#include <iostream>
#include <csignal>

server::App* global_app = nullptr;

void handle_signal(int signum) {
    LOG_INFO("Received signal " << signum << ". Initiating graceful shutdown...");
    if (global_app) {
        global_app->stop();
    }
}

int main(int argc, char* argv[]) {
    try {
        auto config = config::ServerConfig::parse(argc, argv);
        
        server::App app(config);
        global_app = &app;

        std::signal(SIGINT, handle_signal);
        std::signal(SIGTERM, handle_signal);

        // Fluent routing
        app.get("/", [](const http::HttpRequest& req, http::HttpResponse& res) {
            res.html("<h1>Welcome to the C++ Web Framework!</h1>");
        })
        .get("/api/data", [](const http::HttpRequest& req, http::HttpResponse& res) {
            res.json(R"({"status": "success", "message": "epoll is fast!"})");
        })
        .get("/users/:id", [](const http::HttpRequest& req, http::HttpResponse& res) {
            std::string user_id = req.params.at("id");
            res.json("{\"user_id\": \"" + user_id + "\", \"name\": \"John Doe\"}");
        })
        .static_dir(config.static_dir);

        app.listen();
        
        global_app = nullptr;

    } catch (const std::exception& e) {
        LOG_ERROR("Fatal error: " << e.what());
        return 1;
    }

    LOG_INFO("Server shutdown complete. Goodbye!");
    return 0;
}
