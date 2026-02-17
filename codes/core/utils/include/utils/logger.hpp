#pragma once

#include <cstdint>
#include <string>
#include <fstream>
#include <mutex>
#include <atomic>
#include <memory>
#include <cstdarg>

namespace https_server_sim {
namespace utils {

// 日志级别枚举
enum class LogLevel : uint8_t {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

// 日志级别转字符串
const char* log_level_to_string(LogLevel level);

// 字符串转日志级别
LogLevel string_to_log_level(const std::string& str);

class Logger {
public:
    // 获取单例
    static Logger& instance();

    // ========== 初始化与配置 ==========

    // 初始化日志系统
    // level: 日志级别
    // file: 日志文件路径，为空则只输出到控制台
    // return: 0-成功，非0-错误码
    int init(const std::string& level, const std::string& file = "");

    // 初始化日志系统
    // level: 日志级别枚举
    // file: 日志文件路径
    // return: 0-成功
    int init(LogLevel level, const std::string& file = "");

    // 设置日志级别
    void set_level(LogLevel level);

    // 获取当前日志级别
    LogLevel get_level() const;

    // 设置日志文件
    // file: 文件路径，为空关闭文件输出
    // return: 0-成功
    int set_file(const std::string& file);

    // 启用/禁用控制台输出
    void set_console_output(bool enabled);

    // 设置日志格式
    // format: 格式字符串，支持以下占位符:
    //   %time - 时间
    //   %level - 日志级别
    //   %module - 模块名
    //   %message - 日志消息
    //   %thread - 线程ID
    // 默认格式: "[%time] [%level] [%module] %message"
    void set_format(const std::string& format);

    // ========== 日志输出 ==========

    // 输出日志
    // level: 日志级别
    // module: 模块名
    // fmt: printf风格格式字符串
    void log(LogLevel level, const char* module, const char* fmt, ...);

    // 便捷方法
    void debug(const char* module, const char* fmt, ...);
    void info(const char* module, const char* fmt, ...);
    void warn(const char* module, const char* fmt, ...);
    void error(const char* module, const char* fmt, ...);

    // 检查某级别是否启用
    bool is_level_enabled(LogLevel level) const;

    // ========== 刷新与关闭 ==========

    // 刷新日志
    void flush();

    // 关闭日志系统
    void shutdown();

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // 内部实现
    void logv(LogLevel level, const char* module, const char* fmt, va_list args);
    void format_and_write(LogLevel level, const char* module, const char* message);
    std::string format_time() const;

    std::atomic<LogLevel> level_;
    std::mutex mutex_;
    std::ofstream file_;
    bool console_enabled_;
    std::string format_;
    bool initialized_;
};

} // namespace utils
} // namespace https_server_sim

// ========== 便捷宏 ==========

#define LOG_DEBUG(module, fmt, ...) \
    do { \
        auto& logger = ::https_server_sim::utils::Logger::instance(); \
        if (logger.is_level_enabled(::https_server_sim::utils::LogLevel::DEBUG)) { \
            logger.debug(module, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define LOG_INFO(module, fmt, ...) \
    do { \
        auto& logger = ::https_server_sim::utils::Logger::instance(); \
        if (logger.is_level_enabled(::https_server_sim::utils::LogLevel::INFO)) { \
            logger.info(module, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define LOG_WARN(module, fmt, ...) \
    do { \
        auto& logger = ::https_server_sim::utils::Logger::instance(); \
        if (logger.is_level_enabled(::https_server_sim::utils::LogLevel::WARN)) { \
            logger.warn(module, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define LOG_ERROR(module, fmt, ...) \
    do { \
        auto& logger = ::https_server_sim::utils::Logger::instance(); \
        if (logger.is_level_enabled(::https_server_sim::utils::LogLevel::ERROR)) { \
            logger.error(module, fmt, ##__VA_ARGS__); \
        } \
    } while(0)
