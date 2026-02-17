#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace https_server_sim {
namespace utils {

// ========== 时间获取函数 ==========

// 获取当前时间戳（毫秒）
// return: 自Unix纪元以来的毫秒数
uint64_t get_current_time_ms();

// 获取当前时间戳（微秒）
// return: 自Unix纪元以来的微秒数
uint64_t get_current_time_us();

// 获取当前时间戳（纳秒）
// return: 自Unix纪元以来的纳秒数
uint64_t get_current_time_ns();

// 获取单调时间戳（毫秒，不受系统时间修改影响）
// return: 单调递增的毫秒数
uint64_t get_monotonic_time_ms();

// 获取单调时间戳（微秒）
uint64_t get_monotonic_time_us();

// ========== 时间格式化 ==========

// 格式化当前时间为字符串
// format: strftime格式字符串，默认 "%Y-%m-%d %H:%M:%S"
// return: 格式化后的时间字符串
std::string format_current_time(const char* format = "%Y-%m-%d %H:%M:%S");

// 格式化指定时间戳为字符串
// timestamp_ms: 毫秒时间戳
// format: strftime格式字符串
// return: 格式化后的时间字符串
std::string format_time(uint64_t timestamp_ms, const char* format = "%Y-%m-%d %H:%M:%S");

// ========== 时间工具类 ==========

// 计时器类，用于测量耗时
class StopWatch {
public:
    StopWatch();
    ~StopWatch() = default;

    // 重置计时器
    void reset();

    // 获取从构造/重置到现在的毫秒数
    uint64_t elapsed_ms() const;

    // 获取从构造/重置到现在的微秒数
    uint64_t elapsed_us() const;

    // 获取从构造/重置到现在的纳秒数
    uint64_t elapsed_ns() const;

private:
    std::chrono::steady_clock::time_point start_;
};

// 超时检测器
class TimeoutChecker {
public:
    // timeout_ms: 超时时间（毫秒）
    explicit TimeoutChecker(uint64_t timeout_ms);
    ~TimeoutChecker() = default;

    // 检查是否超时
    bool is_timeout() const;

    // 获取剩余时间（毫秒），返回0表示已超时
    uint64_t remaining_ms() const;

    // 重置超时时间
    void reset();

    // 重置为新的超时时间
    void reset(uint64_t new_timeout_ms);

private:
    uint64_t timeout_ms_;
    uint64_t start_time_ms_;
};

} // namespace utils
} // namespace https_server_sim
