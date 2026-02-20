#include "utils/statistics.hpp"
#include <algorithm>
#include <numeric>

namespace https_server_sim {
namespace utils {

StatisticsManager& StatisticsManager::instance() {
    static StatisticsManager mgr;
    return mgr;
}

StatisticsManager::StatisticsManager()
    : total_connections_(0)
    , current_connections_(0)
    , total_requests_(0)
    , total_bytes_received_(0)
    , total_bytes_sent_(0)
    , requests_last_second_(0)
    , requests_current_second_(0)
    , requests_per_second_(0)
    , avg_latency_ms_(0)
    , p50_latency_ms_(0)
    , p95_latency_ms_(0)
    , p99_latency_ms_(0)
{
    latencies_.reserve(MAX_LATENCIES);
}

StatisticsManager::~StatisticsManager() = default;

void StatisticsManager::record_connection() {
    total_connections_.fetch_add(1, std::memory_order_relaxed);
    current_connections_.fetch_add(1, std::memory_order_relaxed);
}

void StatisticsManager::record_connection_close() {
    uint32_t current = current_connections_.load(std::memory_order_relaxed);
    while (current > 0 && !current_connections_.compare_exchange_weak(
        current, current - 1, std::memory_order_relaxed)) {
        // 重试，直到成功或 current 变为 0
    }
}

void StatisticsManager::record_request() {
    total_requests_.fetch_add(1, std::memory_order_relaxed);
    requests_current_second_.fetch_add(1, std::memory_order_relaxed);
}

void StatisticsManager::record_latency(uint32_t latency_ms) {
    std::lock_guard<std::mutex> lock(latencies_mutex_);

    if (latencies_.size() >= MAX_LATENCIES) {
        // 缓冲区满，滑动窗口：移除旧的一半，保留新的一半
        size_t half = MAX_LATENCIES / 2;
        std::copy(latencies_.begin() + half, latencies_.end(), latencies_.begin());
        latencies_.resize(half);
    }
    latencies_.push_back(latency_ms);
}

void StatisticsManager::record_bytes_received(uint64_t bytes) {
    total_bytes_received_.fetch_add(bytes, std::memory_order_relaxed);
}

void StatisticsManager::record_bytes_sent(uint64_t bytes) {
    total_bytes_sent_.fetch_add(bytes, std::memory_order_relaxed);
}

void StatisticsManager::get_statistics(Statistics* stats) {
    if (stats == nullptr) {
        return;
    }

    stats->total_connections = total_connections_.load(std::memory_order_relaxed);
    stats->total_requests = total_requests_.load(std::memory_order_relaxed);
    stats->requests_per_second = requests_per_second_.load(std::memory_order_relaxed);
    stats->total_bytes_received = total_bytes_received_.load(std::memory_order_relaxed);
    stats->total_bytes_sent = total_bytes_sent_.load(std::memory_order_relaxed);

    // 复制延迟统计
    {
        std::lock_guard<std::mutex> lock(latencies_mutex_);
        stats->avg_response_latency_ms = avg_latency_ms_;
        stats->p50_response_latency_ms = p50_latency_ms_;
        stats->p95_response_latency_ms = p95_latency_ms_;
        stats->p99_response_latency_ms = p99_latency_ms_;
    }
}

uint32_t StatisticsManager::current_connections() const {
    return current_connections_.load(std::memory_order_relaxed);
}

uint64_t StatisticsManager::total_connections() const {
    return total_connections_.load(std::memory_order_relaxed);
}

uint64_t StatisticsManager::total_requests() const {
    return total_requests_.load(std::memory_order_relaxed);
}

void StatisticsManager::update() {
    // 更新RPS
    uint64_t current = requests_current_second_.exchange(0, std::memory_order_relaxed);
    requests_per_second_.store(current, std::memory_order_relaxed);
    requests_last_second_.store(current, std::memory_order_relaxed);

    // 计算延迟百分位
    calculate_percentiles();
}

void StatisticsManager::reset() {
    total_connections_.store(0, std::memory_order_relaxed);
    current_connections_.store(0, std::memory_order_relaxed);
    total_requests_.store(0, std::memory_order_relaxed);
    total_bytes_received_.store(0, std::memory_order_relaxed);
    total_bytes_sent_.store(0, std::memory_order_relaxed);
    requests_last_second_.store(0, std::memory_order_relaxed);
    requests_current_second_.store(0, std::memory_order_relaxed);
    requests_per_second_.store(0, std::memory_order_relaxed);

    std::lock_guard<std::mutex> lock(latencies_mutex_);
    latencies_.clear();
    avg_latency_ms_ = 0;
    p50_latency_ms_ = 0;
    p95_latency_ms_ = 0;
    p99_latency_ms_ = 0;
}

void StatisticsManager::calculate_percentiles() {
    std::lock_guard<std::mutex> lock(latencies_mutex_);

    if (latencies_.empty()) {
        avg_latency_ms_ = 0;
        p50_latency_ms_ = 0;
        p95_latency_ms_ = 0;
        p99_latency_ms_ = 0;
        return;
    }

    std::vector<uint32_t> sorted = latencies_;
    std::sort(sorted.begin(), sorted.end());

    // 平均值
    uint64_t sum = std::accumulate(sorted.begin(), sorted.end(), 0ULL);
    avg_latency_ms_ = static_cast<uint32_t>(sum / sorted.size());

    // 百分位
    size_t n = sorted.size();
    // 使用标准百分位计算方法：(n - 1) * P / 100
    // 确保索引范围在 [0, n-1] 之间
    size_t idx_p50 = static_cast<size_t>((n - 1) * 50 / 100);
    size_t idx_p95 = static_cast<size_t>((n - 1) * 95 / 100);
    size_t idx_p99 = static_cast<size_t>((n - 1) * 99 / 100);
    p50_latency_ms_ = sorted[idx_p50];
    p95_latency_ms_ = sorted[idx_p95];
    p99_latency_ms_ = sorted[idx_p99];
}

} // namespace utils
} // namespace https_server_sim
