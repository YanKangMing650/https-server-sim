#include "utils/logger.hpp"
#include "utils/time.hpp"
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>

namespace https_server_sim {
namespace utils {

const char* log_level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

LogLevel string_to_log_level(const std::string& str) {
    if (str == "DEBUG") return LogLevel::DEBUG;
    if (str == "INFO")  return LogLevel::INFO;
    if (str == "WARN")  return LogLevel::WARN;
    if (str == "ERROR") return LogLevel::ERROR;
    return LogLevel::INFO;  // 默认INFO
}

Logger::Logger()
    : level_(LogLevel::INFO)
    , console_enabled_(true)
    , format_("[%time] [%level] [%module] %message")
    , initialized_(false)
{
}

Logger::~Logger() {
    shutdown();
}

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

int Logger::init(const std::string& level, const std::string& file) {
    return init(string_to_log_level(level), file);
}

int Logger::init(LogLevel level, const std::string& file) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        shutdown();
    }

    level_ = level;

    if (!file.empty()) {
        file_.open(file, std::ios::out | std::ios::app);
        if (!file_.is_open()) {
            return -1;  // 打开文件失败
        }
    }

    initialized_ = true;
    return 0;
}

void Logger::set_level(LogLevel level) {
    level_ = level;
}

LogLevel Logger::get_level() const {
    return level_.load();
}

int Logger::set_file(const std::string& file) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (file_.is_open()) {
        file_.close();
    }

    if (file.empty()) {
        return 0;
    }

    file_.open(file, std::ios::out | std::ios::app);
    return file_.is_open() ? 0 : -1;
}

void Logger::set_console_output(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    console_enabled_ = enabled;
}

void Logger::set_format(const std::string& format) {
    std::lock_guard<std::mutex> lock(mutex_);
    format_ = format;
}

bool Logger::is_level_enabled(LogLevel level) const {
    return static_cast<uint8_t>(level) >= static_cast<uint8_t>(level_.load());
}

void Logger::log(LogLevel level, const char* module, const char* fmt, ...) {
    if (!is_level_enabled(level)) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    logv(level, module, fmt, args);
    va_end(args);
}

void Logger::debug(const char* module, const char* fmt, ...) {
    if (!is_level_enabled(LogLevel::DEBUG)) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    logv(LogLevel::DEBUG, module, fmt, args);
    va_end(args);
}

void Logger::info(const char* module, const char* fmt, ...) {
    if (!is_level_enabled(LogLevel::INFO)) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    logv(LogLevel::INFO, module, fmt, args);
    va_end(args);
}

void Logger::warn(const char* module, const char* fmt, ...) {
    if (!is_level_enabled(LogLevel::WARN)) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    logv(LogLevel::WARN, module, fmt, args);
    va_end(args);
}

void Logger::error(const char* module, const char* fmt, ...) {
    if (!is_level_enabled(LogLevel::ERROR)) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    logv(LogLevel::ERROR, module, fmt, args);
    va_end(args);
}

void Logger::logv(LogLevel level, const char* module, const char* fmt, va_list args) {
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    std::lock_guard<std::mutex> lock(mutex_);
    format_and_write(level, module, buffer);
}

void Logger::format_and_write(LogLevel level, const char* module, const char* message) {
    std::string result = format_;

    // 替换占位符
    std::string time_str = format_time();
    std::string level_str = log_level_to_string(level);
    std::string module_str = module ? module : "unknown";

    // 简单的字符串替换
    size_t pos;

    // %time
    pos = result.find("%time");
    if (pos != std::string::npos) {
        result.replace(pos, 5, time_str);
    }

    // %level
    pos = result.find("%level");
    if (pos != std::string::npos) {
        result.replace(pos, 6, level_str);
    }

    // %module
    pos = result.find("%module");
    if (pos != std::string::npos) {
        result.replace(pos, 7, module_str);
    }

    // %thread
    pos = result.find("%thread");
    if (pos != std::string::npos) {
        std::ostringstream oss;
        oss << std::this_thread::get_id();
        result.replace(pos, 7, oss.str());
    }

    // %message
    pos = result.find("%message");
    if (pos != std::string::npos) {
        result.replace(pos, 8, message);
    }

    result += "\n";

    // 输出到控制台
    if (console_enabled_) {
        if (level >= LogLevel::WARN) {
            std::cerr << result;
        } else {
            std::cout << result;
        }
    }

    // 输出到文件
    if (file_.is_open()) {
        file_ << result;
        if (level >= LogLevel::WARN) {
            file_.flush();
        }
    }
}

std::string Logger::format_time() const {
    return format_current_time("%Y-%m-%d %H:%M:%S");
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.flush();
    }
    std::cout.flush();
    std::cerr.flush();
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
    initialized_ = false;
}

} // namespace utils
} // namespace https_server_sim
