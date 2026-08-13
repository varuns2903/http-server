#include <orbit/middleware/SessionManager.hpp>
#include <random>
#include <sstream>

namespace middleware {

SessionManager::SessionManager(const std::string& redis_host, int redis_port)
    : redis_(std::make_shared<database::RedisClient>(redis_host, redis_port)) {}

std::string SessionManager::generate_session_id() {
    thread_local std::random_device rd;
    thread_local std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    const char* hex = "0123456789abcdef";
    
    std::string uuid = "";
    for (int i = 0; i < 32; i++) {
        uuid += hex[dis(gen)];
    }
    return uuid;
}


bool SessionManager::operator()(http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer) {
    std::string session_id;
    auto it = req.cookies.find("session_id");
    if (it != req.cookies.end()) {
        session_id = it->second;
    }
    
    if (session_id.empty()) {
        session_id = generate_session_id();
        writer->add_interceptor([session_id](http::HttpResponse& res) {
            http::Cookie c;
            c.name = "session_id";
            c.value = session_id;
            c.path = "/";
            c.http_only = true;
            res.set_cookie(c);
        });
    }
    
    req.session_id = session_id;
    return true; // Continue pipeline
}

} // namespace middleware
