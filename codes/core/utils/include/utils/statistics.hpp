#pragma once

#include <cstdint>
#include <vector>
#include <atomic>
#include <mutex>

namespace https_server_sim {
namespace utils {

// 统计数据结构
struct Statistics {
    // 连接统计
    uint64_t total_connections;
    uint64_t active_connections;
    uint64_t peak_connections;

    // 请求统计
    uint64_t total_requests;
    uint64_t requests_per_second;

    // 数据传输统计
    uint64_t bytes_received;
    uint64_t bytes_sent;

    // 延迟统计（毫秒）
    uint64_t min_latency_ms;
    uint64_t max_latency_ms;
    uint64_t avg_latency_ms;
    uint64_t p50_latency_ms;
    uint64_t p95_latency_ms;
    uint64_t p99_latency_ms;

    // 错误统计
    uint64_t error_count;

    Statistics();
    void reset();
};

// 统计管理器
class StatisticsManager {
public:
    static StatisticsManager& instance();

    // ========== 记录统计数据 ==========

    // 记录连接
    void record_connection_open();
    void record_connection_close();

    // 记录请求
    void record_request(uint64_t latency_ms);

    // 记录数据传输
    void record_bytes_received(uint64_t bytes);
    void record_bytes_sent(uint64_t bytes);

    // 记录错误
    void record_error();

    // ========== 获取统计数据 ==========

    // 获取当前统计快照
    Statistics get_statistics() const;

    // 重置所有统计
    void reset();

private:
    StatisticsManager();
    ~StatisticsManager();
    StatisticsManager(const StatisticsManager&) = delete;
    StatisticsManager& operator=(const StatisticsManager&) = delete;

    // 记录延迟样本（用于计算百分位）
    void add_latency_sample(uint64_t latency_ms);
    void calculate_percentiles(Statistics& stats) const;

    mutable std::mutex mutex_;

    // 基础统计（原子变量，无锁访问）
    std::atomic<uint64_t> total_connections_;
    std::atomic<uint64_t> active_connections_;
    std::atomic<uint64_t> peak_connections_;
    std::atomic<uint64_t> total_requests_;
    std::atomic<uint64_t> bytes_received_;
    std::atomic<uint64_t> bytes_sent_;
    std::atomic<uint64_t> error_count_;

    // 延迟统计（需要锁保护）
    std::vector<uint64_t> latency_samples_;
    static constexpr size_t MAX_LATENCY_SAMPLES = 10000;
    uint64_t latency_sum_;
    uint64_t latency_min_;
    uint64_t latency_max_;
};

} // namespace utils
} // namespace https_server_sim
