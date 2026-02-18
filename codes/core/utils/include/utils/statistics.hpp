#pragma once

#include <cstdint>
#include <atomic>
#include <vector>
#include <mutex>

namespace https_server_sim {
namespace utils {

// 统计信息结构体
struct Statistics {
    uint64_t total_connections = 0;
    uint64_t total_requests = 0;
    uint64_t requests_per_second = 0;
    uint32_t avg_response_latency_ms = 0;
    uint32_t p50_response_latency_ms = 0;
    uint32_t p95_response_latency_ms = 0;
    uint32_t p99_response_latency_ms = 0;
    uint64_t total_bytes_received = 0;
    uint64_t total_bytes_sent = 0;
};

class StatisticsManager {
public:
    static StatisticsManager& instance();

    StatisticsManager();
    ~StatisticsManager();

    // 禁止拷贝
    StatisticsManager(const StatisticsManager&) = delete;
    StatisticsManager& operator=(const StatisticsManager&) = delete;

    // ========== 统计记录 ==========

    // 记录新连接
    void record_connection();

    // 记录连接关闭
    void record_connection_close();

    // 记录请求
    void record_request();

    // 记录响应延迟
    void record_latency(uint32_t latency_ms);

    // 记录接收字节
    void record_bytes_received(uint64_t bytes);

    // 记录发送字节
    void record_bytes_sent(uint64_t bytes);

    // ========== 获取统计 ==========

    // 获取统计信息
    void get_statistics(Statistics* stats);

    // 获取当前连接数
    uint32_t current_connections() const;

    // 获取总连接数
    uint64_t total_connections() const;

    // 获取总请求数
    uint64_t total_requests() const;

    // ========== 更新与重置 ==========

    // 定期更新（每秒调用）
    void update();

    // 重置所有统计
    void reset();

private:
    // 计算百分位延迟
    void calculate_percentiles();

    std::atomic<uint64_t> total_connections_;
    std::atomic<uint32_t> current_connections_;
    std::atomic<uint64_t> total_requests_;
    std::atomic<uint64_t> total_bytes_received_;
    std::atomic<uint64_t> total_bytes_sent_;

    // RPS计算
    std::atomic<uint64_t> requests_last_second_;
    std::atomic<uint64_t> requests_current_second_;
    std::atomic<uint64_t> requests_per_second_;

    // 延迟统计
    // 最大延迟记录数：选择100000是为了在内存占用和统计准确性之间取得平衡
    // 假设每个延迟4字节，约占用400KB内存，可以覆盖典型的统计需求
    static constexpr size_t MAX_LATENCIES = 100000;
    std::vector<uint32_t> latencies_;
    std::mutex latencies_mutex_;

    uint32_t avg_latency_ms_;
    uint32_t p50_latency_ms_;
    uint32_t p95_latency_ms_;
    uint32_t p99_latency_ms_;
};

} // namespace utils
} // namespace https_server_sim
