#pragma once
#include <string>
#include <memory>
#include <orbit/http/HttpRequest.hpp>
#include <orbit/http/ResponseWriter.hpp>
#include <orbit/database/RedisClient.hpp>

namespace middleware {

class SessionManager {
public:
    SessionManager(const std::string& redis_host, int redis_port);
    
    bool operator()(http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer);

    // Static helper to get and set session data using an independent redis connection 
    // or passing the manager's connection. But for simplicity, users will use a global DB connection.
private:
    std::string generate_session_id();

    std::shared_ptr<database::RedisClient> redis_;
};

inline auto session(const std::string& redis_host, int redis_port) {
    return [manager = std::make_shared<SessionManager>(redis_host, redis_port)]
           (http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer) -> bool {
        return (*manager)(req, writer);
    };
}

} // namespace middleware
