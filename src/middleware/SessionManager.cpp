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

std::string SessionManager::extract_cookie(const std::string_view& cookie_header, const std::string& name) {
    std::string cookies(cookie_header);
    std::istringstream stream(cookies);
    std::string cookie;
    while (std::getline(stream, cookie, ';')) {
        // trim leading spaces
        size_t start = cookie.find_first_not_of(" ");
        if (start != std::string::npos) {
            cookie = cookie.substr(start);
        }
        
        size_t eq = cookie.find('=');
        if (eq != std::string::npos) {
            std::string key = cookie.substr(0, eq);
            if (key == name) {
                return cookie.substr(eq + 1);
            }
        }
    }
    return "";
}

bool SessionManager::operator()(http::HttpRequest& req, std::shared_ptr<http::ResponseWriter> writer) {
    std::string session_id;
    
    auto it = req.headers.find("Cookie");
    if (it != req.headers.end()) {
        session_id = extract_cookie(it->second, "session_id");
    }
    
    if (session_id.empty()) {
        session_id = generate_session_id();
        writer->set_header("Set-Cookie", "session_id=" + session_id + "; Path=/; HttpOnly");
    }
    
    req.session_id = session_id;
    return true; // Continue pipeline
}

} // namespace middleware
