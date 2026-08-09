#pragma once
#include <string>
#include <cstdint>

namespace config {

struct ServerConfig {
    uint16_t port{8080};
    size_t worker_threads{4};
    std::string log_level{"INFO"};
    std::string static_dir{"./public"};
    size_t max_body_size{10485760}; // Default 10 MB limit
    std::string ssl_cert{""};
    std::string ssl_key{""};
    
    static ServerConfig parse(int argc, char* argv[]);
};

} // namespace config
