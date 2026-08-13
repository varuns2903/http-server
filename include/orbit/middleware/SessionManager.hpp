#pragma once
#include <string>
#include <memory>
#include <orbit/http/HttpRequest.hpp>
#include <orbit/http/ResponseWriter.hpp>
#include <orbit/database/RedisClient.hpp>

namespace middleware {

/**
 * @ingroup middlewares
 * @brief Middleware for managing sessions using Redis.
 */
class SessionManager {
public:
    /**
     * @brief Constructs a new Session Manager.
     * 
     * @param redis_host The Redis server host.
     * @param redis_port The Redis server port.
     */
    SessionManager(const std::string& redis_host, int redis_port);
    
    /**
     * @brief Middleware execution operator.
     * 
     * @param req The HTTP request.
     * @param writer The HTTP response writer.
     * @return true If the request should continue to the next handler.
     * @return false If the request should be blocked.
     */
    bool operator()(http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer);

private:
    std::string generate_session_id();

    std::shared_ptr<database::RedisClient> redis_;
};

/**
 * @ingroup middlewares
 * @brief Helper to create a session management middleware handler.
 * 
 * @param redis_host The Redis server host.
 * @param redis_port The Redis server port.
 * @return A middleware handler function.
 */
inline auto session(const std::string& redis_host, int redis_port) {
    return [manager = std::make_shared<SessionManager>(redis_host, redis_port)]
           (http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer) -> bool {
        return (*manager)(req, writer);
    };
}

} // namespace middleware
