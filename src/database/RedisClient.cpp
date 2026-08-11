#include "RedisClient.hpp"
#include "../utils/Logger.hpp"
#include "../network/PlatformSocket.hpp"
#include <sstream>

namespace database {

RedisClient::RedisClient(const std::string& host, int port) : host_(host), port_(port) {
    connect();
}

RedisClient::~RedisClient() {
    disconnect();
}

bool RedisClient::connect() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (fd_ != -1) return true;

    struct hostent* he = gethostbyname(host_.c_str());
    if (!he) {
        LOG_ERROR("Redis DNS resolution failed for " << host_);
        return false;
    }

    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0) return false;

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr = *(struct in_addr*)he->h_addr_list[0];

    // Connect synchronously (this runs on worker threads, so it's okay)
    if (::connect(fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        network::close_socket(fd_);
        fd_ = -1;
        LOG_ERROR("Failed to connect to Redis at " << host_ << ":" << port_);
        return false;
    }

    LOG_INFO("Connected to Redis successfully");
    return true;
}

void RedisClient::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (fd_ != -1) {
        network::close_socket(fd_);
        fd_ = -1;
    }
}

std::string RedisClient::send_command(const std::vector<std::string>& args) {
    std::ostringstream oss;
    oss << "*" << args.size() << "\r\n";
    for (const auto& arg : args) {
        oss << "$" << arg.size() << "\r\n" << arg << "\r\n";
    }
    
    std::string req = oss.str();
    
    std::lock_guard<std::mutex> lock(mutex_);
    if (fd_ == -1 && !connect()) return "";

    size_t total_sent = 0;
    while (total_sent < req.size()) {
        ssize_t s = send(fd_, req.data() + total_sent, req.size() - total_sent, 0);
        if (s <= 0) {
            network::close_socket(fd_);
            fd_ = -1;
            return ""; // Connection dropped
        }
        total_sent += s;
    }

    return read_response();
}

std::string RedisClient::read_response() {
    // A simple, unoptimized RESP reader for synchronous reading.
    char c;
    std::string line;
    
    // Read the first line to determine response type
    while (recv(fd_, &c, 1, 0) == 1) {
        line += c;
        if (line.size() >= 2 && line.substr(line.size() - 2) == "\r\n") {
            break;
        }
    }

    if (line.empty()) return "";

    char type = line[0];
    line = line.substr(1, line.size() - 3); // Remove type char and \r\n

    if (type == '+') {
        // Simple string
        return line;
    } else if (type == '-') {
        // Error
        LOG_ERROR("Redis Error: " << line);
        return "";
    } else if (type == ':') {
        // Integer
        return line;
    } else if (type == '$') {
        // Bulk string
        long long len = std::stoll(line);
        if (len == -1) return ""; // Null
        
        std::string bulk;
        bulk.resize(len);
        
        size_t total_read = 0;
        while (total_read < len) {
            ssize_t r = recv(fd_, &bulk[0] + total_read, len - total_read, 0);
            if (r <= 0) return "";
            total_read += r;
        }
        
        // consume trailing \r\n
        char crlf[2];
        recv(fd_, crlf, 2, 0);
        
        return bulk;
    } else if (type == '*') {
        // Arrays not strictly needed yet for basic session management, returning stringified count
        return "ARRAY:" + line; 
    }

    return "";
}

std::string RedisClient::ping() {
    return send_command({"PING"});
}

bool RedisClient::set(const std::string& key, const std::string& value, int expire_seconds) {
    if (expire_seconds > 0) {
        std::string res = send_command({"SET", key, value, "EX", std::to_string(expire_seconds)});
        return res == "OK";
    } else {
        std::string res = send_command({"SET", key, value});
        return res == "OK";
    }
}

std::optional<std::string> RedisClient::get(const std::string& key) {
    std::string res = send_command({"GET", key});
    if (res.empty()) return std::nullopt; // nullopt or actual empty based on logic
    return res;
}

long long RedisClient::incr(const std::string& key) {
    std::string res = send_command({"INCR", key});
    if (res.empty()) return 0;
    try {
        return std::stoll(res);
    } catch (...) {
        return 0;
    }
}

bool RedisClient::expire(const std::string& key, int seconds) {
    std::string res = send_command({"EXPIRE", key, std::to_string(seconds)});
    return res == "1";
}

} // namespace database
