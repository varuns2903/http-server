#include "Logger.hpp"
#include <chrono>
#include <iomanip>

namespace utils {

LogLevel Logger::current_level = LogLevel::INFO;
std::mutex Logger::log_mutex;

void Logger::init(const std::string& level_str) {
    if (level_str == "DEBUG") current_level = LogLevel::DEBUG;
    else if (level_str == "INFO") current_level = LogLevel::INFO;
    else if (level_str == "WARN") current_level = LogLevel::WARN;
    else if (level_str == "ERROR") current_level = LogLevel::ERROR;
}

std::string Logger::level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "\033[36mDEBUG\033[0m"; // Cyan
        case LogLevel::INFO:  return "\033[32mINFO \033[0m"; // Green
        case LogLevel::WARN:  return "\033[33mWARN \033[0m"; // Yellow
        case LogLevel::ERROR: return "\033[31mERROR\033[0m"; // Red
        default: return "UNKNOWN";
    }
}

void Logger::log(LogLevel level, const char* file, int line, const std::string& msg) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    // Extract filename from path
    std::string file_str(file);
    size_t last_slash = file_str.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        file_str = file_str.substr(last_slash + 1);
    }
    
    std::lock_guard<std::mutex> lock(log_mutex);
#ifdef _WIN32
    struct tm ti;
    localtime_s(&ti, &time);
    struct tm* ti_ptr = &ti;
#else
    struct tm* ti_ptr = std::localtime(&time);
#endif
    std::cout << "[" << std::put_time(ti_ptr, "%Y-%m-%d %H:%M:%S") 
              << "." << std::setfill('0') << std::setw(3) << ms.count() << "] "
              << "[" << level_to_string(level) << "] "
              << "[" << file_str << ":" << line << "] "
              << msg << "\n";
}

} // namespace utils
