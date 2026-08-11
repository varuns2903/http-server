#pragma once
#ifdef _WIN32
#undef ERROR
#endif
#include <string>
#include <iostream>
#include <sstream>
#include <mutex>

namespace utils {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

class Logger {
public:
    static void init(const std::string& level_str);
    static void log(LogLevel level, const char* file, int line, const std::string& msg);
    
    static LogLevel current_level;
private:
    static std::mutex log_mutex;
    static std::string level_to_string(LogLevel level);
};

} // namespace utils

#define LOG_DEBUG(msg) do { if (utils::Logger::current_level <= utils::LogLevel::DEBUG) { std::ostringstream oss; oss << msg; utils::Logger::log(utils::LogLevel::DEBUG, __FILE__, __LINE__, oss.str()); } } while(0)
#define LOG_INFO(msg)  do { if (utils::Logger::current_level <= utils::LogLevel::INFO)  { std::ostringstream oss; oss << msg; utils::Logger::log(utils::LogLevel::INFO, __FILE__, __LINE__, oss.str()); } } while(0)
#define LOG_WARN(msg)  do { if (utils::Logger::current_level <= utils::LogLevel::WARN)  { std::ostringstream oss; oss << msg; utils::Logger::log(utils::LogLevel::WARN, __FILE__, __LINE__, oss.str()); } } while(0)
#define LOG_ERROR(msg) do { if (utils::Logger::current_level <= utils::LogLevel::ERROR) { std::ostringstream oss; oss << msg; utils::Logger::log(utils::LogLevel::ERROR, __FILE__, __LINE__, oss.str()); } } while(0)
