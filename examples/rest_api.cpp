#include "server/App.hpp"
#include <iostream>
#include <vector>

struct User {
    int id;
    std::string name;
    std::string email;
};

int main() {
    config::ServerConfig cfg;
    cfg.port = 8080;
    server::App app(cfg);

    // Mock database
    std::vector<User> db = {
        {1, "Alice", "alice@example.com"},
        {2, "Bob", "bob@example.com"}
    };

    // Global Middleware for JSON API
    app.use([](http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer) {
        writer->set_header("Content-Type", "application/json");
        return true; // continue
    });

    app.get("/api/users", [&db](const http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer) {
        nlohmann::json res_json = nlohmann::json::array();
        for (const auto& u : db) {
            res_json.push_back({{"id", u.id}, {"name", u.name}, {"email", u.email}});
        }
        
        http::HttpResponse res;
        res.status_code = http::HttpStatus::OK;
        res.set_body(res_json.dump(4));
        writer->send(std::move(res));
    });

    app.post("/api/users", [&db](const http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer) {
        auto json = req.json();
        
        if (!json.contains("name") || !json.contains("email")) {
            http::HttpResponse res;
            res.status_code = http::HttpStatus::BadRequest;
            res.set_body(nlohmann::json({{"error", "Missing name or email"}}).dump());
            writer->send(std::move(res));
            return;
        }

        User u{static_cast<int>(db.size() + 1), json["name"], json["email"]};
        db.push_back(u);

        http::HttpResponse res;
        res.status_code = http::HttpStatus::Created;
        res.set_body(nlohmann::json({{"id", u.id}, {"message", "User created"}}).dump());
        writer->send(std::move(res));
    });

    std::cout << "Starting REST API on http://localhost:8080" << std::endl;
    app.listen();

    return 0;
}
