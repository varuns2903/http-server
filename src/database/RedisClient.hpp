#pragma once
#include <string>
#include <vector>
#include <optional>
#include <mutex>
#include "../network/Socket.hpp"

namespace database {

class RedisClient {
public:
    RedisClient(const std::string& host, int port);
    ~RedisClient();

    bool connect();
    void disconnect();

    // Core commands
    std::string ping();
    bool set(const std::string& key, const std::string& value, int expire_seconds = 0);
    std::optional<std::string> get(const std::string& key);
    long long incr(const std::string& key);
    bool expire(const std::string& key, int seconds);

private:
    std::string send_command(const std::vector<std::string>& args);
    std::string read_response();

    std::string host_;
    int port_;
    int fd_{-1};
    std::mutex mutex_; // Thread-safe for shared use across worker threads
};

} // namespace database
