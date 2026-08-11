#include "server/App.hpp"
#include "middleware/StaticFiles.hpp"
#include "middleware/Cors.hpp"
#include "middleware/Proxy.hpp"
#include "middleware/RateLimiter.hpp"
#include "middleware/SessionManager.hpp"
#include "middleware/JwtAuth.hpp"
#include "middleware/Metrics.hpp"
#include "database/RedisClient.hpp"
#include "database/PostgresClient.hpp"
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
        app.use([](http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> /*writer*/) {
            LOG_INFO("[Middleware] Received " << req.uri);
            return true;
        });

        // Proxy Middleware for /proxy -> 127.0.0.1:8080
        app.use([](http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer) {
            if (req.uri.find("/proxy") == 0) {
                auto proxy_mw = middleware::proxy("127.0.0.1", 8080);
                return proxy_mw(req, writer);
            }
            return true;
        });

        // Proxy Middleware for /proxy -> 127.0.0.1:8080
        app.use([](http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer) {
            if (req.uri.find("/proxy") == 0) {
                return middleware::proxy("127.0.0.1", 8080)(req, writer);
            }
            return true;
        });

        // Setup global middlewares
        app.use(middleware::cors());
        app.use(middleware::Metrics::track());
        
        // Enable Prometheus metrics endpoint at /metrics
        app.enable_metrics();

        // Serve static files from "public" directory
        app.use(middleware::static_files(config.static_dir));

        // Rate Limiting (100 requests / 10 sec)
        app.use(middleware::rate_limit(100, std::chrono::seconds(10)));

        // Session Manager
        app.use(middleware::session("127.0.0.1", 6379));

        // API Group
        app.group("/api/v1", [](routing::Router& api) {
            api.add_route(http::HttpMethod::GET, "/users", [](const http::HttpRequest& /*req*/, std::shared_ptr<http::ResponseWriter> writer) {
                http::HttpResponse res;
                res.body = "List of users";
                writer->send(std::move(res));
            });
            
            api.add_route(http::HttpMethod::POST, "/users", [](const http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer) {
                auto j = req.json();
                http::HttpResponse res;
                res.status(http::HttpStatus::Created);
                res.body = "User created: " + j.value("name", "Unknown");
                writer->send(std::move(res));
            });
            
            api.add_route(http::HttpMethod::GET, "/users/:id", [](const http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer) {
                http::HttpResponse res;
                res.body = "User ID: " + req.params.at("id");
                writer->send(std::move(res));
            });
        });

        // Fluent routing
        app.get("/", [](const http::HttpRequest& /*req*/, std::shared_ptr<http::ResponseWriter> writer) {
            http::HttpResponse res;
            res.body = "<h1>Welcome to Orbit Framework!</h1>";
            res.headers["Content-Type"] = "text/html";
            writer->send(std::move(res));
        })
        .get("/api/data", [](const http::HttpRequest& /*req*/, std::shared_ptr<http::ResponseWriter> writer) {
            http::HttpResponse res;
            res.body = R"({"status": "success", "data": [1, 2, 3]})";
            res.headers["Content-Type"] = "application/json";
            writer->send(std::move(res));
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
        .get("/users/:id", [](const http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer) {
            std::string user_id = req.params.at("id");
            nlohmann::json j = {
                {"user_id", user_id},
                {"name", "John Doe"}
            };
            http::HttpResponse res;
            res.json(j);
            writer->send(std::move(res));
        })
        .post("/api/echo", [](const http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer) {
            auto j = req.json();
            j["received"] = true;
            http::HttpResponse res;
            res.json(j);
            writer->send(std::move(res));
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
