#include "server/App.hpp"
#include "middleware/StaticFiles.hpp"
#include "middleware/Cors.hpp"
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

        struct sigaction sa;
        sa.sa_handler = handle_signal;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        
        sigaction(SIGINT, &sa, nullptr);
        sigaction(SIGTERM, &sa, nullptr);

        // Global Middleware (Logging)
        app.use([](http::HttpRequest& req, http::HttpResponse& /*res*/) {
            LOG_INFO("[Middleware] Received " << req.uri);
            return true;
        });

        // CORS Middleware
        app.use(middleware::cors());

        // Static File Server Middleware
        app.use(middleware::static_files(config.static_dir));

        // Fluent routing
        app.get("/", [](const http::HttpRequest& /*req*/, http::HttpResponse& res) {
            res.body = "<h1>Welcome to Orbit Framework!</h1>";
            res.headers["Content-Type"] = "text/html";
        })
        .get("/api/data", [](const http::HttpRequest& /*req*/, http::HttpResponse& res) {
            res.body = R"({"status": "success", "data": [1, 2, 3]})";
            res.headers["Content-Type"] = "application/json";
        })
        .ws("/chat", [](http::websocket::WebSocketConnection& ws) {
            ws.on_message([&ws](const std::string& msg) {
                std::cout << "Received WS Message: " << msg << std::endl;
                ws.send("Echo: " + msg);
            });
            ws.on_close([]() {
                std::cout << "WS connection closed." << std::endl;
            });
        })
        .get("/users/:id", [](const http::HttpRequest& req, http::HttpResponse& res) {
            std::string user_id = req.params.at("id");
            nlohmann::json j = {
                {"user_id", user_id},
                {"name", "John Doe"}
            };
            res.json(j);
        })
        .post("/api/echo", [](const http::HttpRequest& req, http::HttpResponse& res) {
            auto j = req.json();
            j["received"] = true;
            res.json(j);
        });

        app.listen();
        
        global_app = nullptr;

    } catch (const std::exception& e) {
        LOG_ERROR("Fatal error: " << e.what());
        return 1;
    }

    LOG_INFO("Server shutdown complete. Goodbye!");
    return 0;
}
