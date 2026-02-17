#include "utils/statistics.hpp"
#include <algorithm>
#include <numeric>

namespace https_server_sim {
namespace utils {

Statistics::Statistics()
    : total_connections(0)
    , active_connections(0)
    , peak_connections(0)
    , total_requests(0)
    , requests_per_second(0)
    , bytes_received(0)
    , bytes_sent(0)
    , min_latency_ms(0)
    , max_latency_ms(0)
    , avg_latency_ms(0)
    , p50_latency_ms(0)
    , p95_latency_ms(0)
    , p99_latency_ms(0)
    , error_count(0)
{
}

void Statistics::reset() {
    total_connections = 0;
    active_connections = 0;
    peak_connections = 0;
    total_requests = 0;
    requests_per_second = 0;
    bytes_received = 0;
    bytes_sent = 0;
    min_latency_ms = 0;
    max_latency_ms = 0;
    avg_latency_ms = 0;
    p50_latency_ms = 0;
    p95_latency_ms = 0;
    p99_latency_ms = 0;
    error_count = 0;
}

StatisticsManager& StatisticsManager::instance() {
    static StatisticsManager manager;
    return manager;
}

StatisticsManager::StatisticsManager()
    : total_connections_(0)
    , active_connections_(0)
    , peak_connections_(0)
    , total_requests_(0)
    , bytes_received_(0)
    , bytes_sent_(0)
    , error_count_(0)
    , latency_sum_(0)
    , latency_min_(UINT64_MAX)
    , latency_max_(0)
{
    latency_samples_.reserve(MAX_LATENCY_SAMPLES);
}

StatisticsManager::~StatisticsManager() {
}

void StatisticsManager::record_connection_open() {
    total_connections_.fetch_add(1, std::memory_order_relaxed);
    uint64_t active = active_connections_.fetch_add(1, std::memory_order_relaxed) + 1;

    // 更新峰值连接数
    uint64_t peak = peak_connections_.load(std::memory_order_relaxed);
    while (active > peak &&
           !peak_connections_.compare_exchange_weak(peak, active,
                                                    std::memory_order_relaxed)) {
    }
}

void StatisticsManager::record_connection_close() {
    active_connections_.fetch_sub(1, std::memory_order_relaxed);
}

void StatisticsManager::record_request(uint64_t latency_ms) {
    total_requests_.fetch_add(1, std::memory_order_relaxed);

    std::lock_guard<std::mutex> lock(mutex_);
    add_latency_sample(latency_ms);
}

void StatisticsManager::record_bytes_received(uint64_t bytes) {
    bytes_received_.fetch_add(bytes, std::memory_order_relaxed);
}

void StatisticsManager::record_bytes_sent(uint64_t bytes) {
    bytes_sent_.fetch_add(bytes, std::memory_order_relaxed);
}

void StatisticsManager::record_error() {
    error_count_.fetch_add(1, std::memory_order_relaxed);
}

Statistics StatisticsManager::get_statistics() const {
    std::lock_guard<std::mutex> lock(mutex_);

    Statistics stats;
    stats.total_connections = total_connections_.load(std::memory_order_relaxed);
    stats.active_connections = active_connections_.load(std::memory_order_relaxed);
    stats.peak_connections = peak_connections_.load(std::memory_order_relaxed);
    stats.total_requests = total_requests_.load(std::memory_order_relaxed);
    stats.bytes_received = bytes_received_.load(std::memory_order_relaxed);
    stats.bytes_sent = bytes_sent_.load(std::memory_order_relaxed);
    stats.error_count = error_count_.load(std::memory_order_relaxed);

    stats.min_latency_ms = latency_min_ == UINT64_MAX ? 0 : latency_min_;
    stats.max_latency_ms = latency_max_;

    if (!latency_samples_.empty()) {
        stats.avg_latency_ms = latency_sum_ / latency_samples_.size();
        calculate_percentiles(stats);
    }

    return stats;
}

void StatisticsManager::reset() {
    std::lock_guard<std::mutex> lock(mutex_);

    total_connections_.store(0, std::memory_order_relaxed);
    active_connections_.store(0, std::memory_order_relaxed);
    peak_connections_.store(0, std::memory_order_relaxed);
    total_requests_.store(0, std::memory_order_relaxed);
    bytes_received_.store(0, std::memory_order_relaxed);
    bytes_sent_.store(0, std::memory_order_relaxed);
    error_count_.store(0, std::memory_order_relaxed);

    latency_samples_.clear();
    latency_sum_ = 0;
    latency_min_ = UINT64_MAX;
    latency_max_ = 0;
}

void StatisticsManager::add_latency_sample(uint64_t latency_ms) {
    // 更新最小/最大
    if (latency_ms < latency_min_) {
        latency_min_ = latency_ms;
    }
    if (latency_ms > latency_max_) {
        latency_max_ = latency_ms;
    }

    // 添加样本
    if (latency_samples_.size() >= MAX_LATENCY_SAMPLES) {
        // 移除最早的样本
        if (!latency_samples_.empty()) {
            latency_sum_ -= latency_samples_.front();
            latency_samples_.erase(latency_samples_.begin());
        }
    }
    latency_samples_.push_back(latency_ms);
    latency_sum_ += latency_ms;
}

void StatisticsManager::calculate_percentiles(Statistics& stats) const {
    if (latency_samples_.empty()) {
        return;
    }

    // 创建副本进行排序
    std::vector<uint64_t> sorted = latency_samples_;
    std::sort(sorted.begin(), sorted.end());

    size_t count = sorted.size();

    // P50
    size_t idx_p50 = count * 50 / 100;
    stats.p50_latency_ms = sorted[std::min(idx_p50, count - 1)];

    // P95
    size_t idx_p95 = count * 95 / 100;
    stats.p95_latency_ms = sorted[std::min(idx_p95, count - 1)];

    // P99
    size_t idx_p99 = count * 99 / 100;
    stats.p99_latency_ms = sorted[std::min(idx_p99, count - 1)];
}

} // namespace utils
} // namespace https_server_sim
