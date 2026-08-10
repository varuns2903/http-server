#pragma once
#include <string>
#include <cstdint>

namespace config {

enum class EventEngine {
    Epoll,
    IoUring
};

struct ServerConfig {
    uint16_t port{8080};
    size_t worker_threads{4};
    std::string log_level{"INFO"};
    std::string static_dir{"./public"};
    size_t max_body_size{10485760}; // Default 10 MB limit
    bool enable_quic{true}; // Enable HTTP/3 QUIC
    std::string ssl_cert{""};
    std::string ssl_key{""};
    EventEngine engine{EventEngine::IoUring}; // Default to our new fast backend!
    
    static ServerConfig parse(int argc, char* argv[]);
};

} // namespace config
