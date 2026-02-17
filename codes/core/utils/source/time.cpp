#include "utils/time.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>

namespace https_server_sim {
namespace utils {

uint64_t get_current_time_ms() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

uint64_t get_current_time_us() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

uint64_t get_current_time_ns() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

uint64_t get_monotonic_time_ms() {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

uint64_t get_monotonic_time_us() {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

std::string format_current_time(const char* format) {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, format);
    return oss.str();
}

std::string format_time(uint64_t timestamp_ms, const char* format) {
    std::time_t time = static_cast<std::time_t>(timestamp_ms / 1000);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, format);
    return oss.str();
}

// StopWatch implementation
StopWatch::StopWatch() {
    reset();
}

void StopWatch::reset() {
    start_ = std::chrono::steady_clock::now();
}

uint64_t StopWatch::elapsed_ms() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_);
    return duration.count();
}

uint64_t StopWatch::elapsed_us() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start_);
    return duration.count();
}

uint64_t StopWatch::elapsed_ns() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(now - start_);
    return duration.count();
}

// TimeoutChecker implementation
TimeoutChecker::TimeoutChecker(uint64_t timeout_ms)
    : timeout_ms_(timeout_ms)
    , start_time_ms_(get_monotonic_time_ms())
{
}

bool TimeoutChecker::is_timeout() const {
    return remaining_ms() == 0;
}

uint64_t TimeoutChecker::remaining_ms() const {
    uint64_t elapsed = get_monotonic_time_ms() - start_time_ms_;
    if (elapsed >= timeout_ms_) {
        return 0;
    }
    return timeout_ms_ - elapsed;
}

void TimeoutChecker::reset() {
    start_time_ms_ = get_monotonic_time_ms();
}

void TimeoutChecker::reset(uint64_t new_timeout_ms) {
    timeout_ms_ = new_timeout_ms;
    start_time_ms_ = get_monotonic_time_ms();
}

} // namespace utils
} // namespace https_server_sim
