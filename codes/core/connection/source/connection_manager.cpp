// =============================================================================
//  HTTPS Server Simulator - Connection Module
//  文件: connection_manager.cpp
//  描述: ConnectionManager 实现
//  版权: Copyright (c) 2026
// =============================================================================
#include "connection/connection_manager.hpp"

namespace https_server_sim {

// ConnectionManager
ConnectionManager::ConnectionManager()
    : next_connection_id_(1)
    , time_source_(std::make_unique<DefaultTimeSource>())
{
}

ConnectionManager::ConnectionManager(std::unique_ptr<TimeSource> time_source)
    : next_connection_id_(1)
    , time_source_(std::move(time_source))
{
}

ConnectionManager::~ConnectionManager() {
    clear_all();
}

Connection* ConnectionManager::create_connection(int fd, uint16_t server_port) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t id = next_connection_id_++;
    auto conn = std::make_unique<Connection>(id, fd, server_port, time_source_.get());
    Connection* ptr = conn.get();
    connections_[id] = std::move(conn);
    return ptr;
}

Connection* ConnectionManager::get_connection(uint64_t conn_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = connections_.find(conn_id);
    return (it != connections_.end()) ? it->second.get() : nullptr;
}

const Connection* ConnectionManager::get_connection(uint64_t conn_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = connections_.find(conn_id);
    return (it != connections_.end()) ? it->second.get() : nullptr;
}

void ConnectionManager::remove_connection(uint64_t conn_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    connections_.erase(conn_id);
}

uint32_t ConnectionManager::get_connection_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<uint32_t>(connections_.size());
}

void ConnectionManager::for_each_connection(std::function<void(Connection&)> func) {
    std::vector<Connection*> conns;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        conns.reserve(connections_.size());
        for (auto& pair : connections_) {
            conns.push_back(pair.second.get());
        }
    }
    for (Connection* conn : conns) {
        func(*conn);
    }
}

void ConnectionManager::for_each_connection(std::function<void(const Connection&)> func) const {
    std::vector<const Connection*> conns;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        conns.reserve(connections_.size());
        for (const auto& pair : connections_) {
            conns.push_back(pair.second.get());
        }
    }
    for (const Connection* conn : conns) {
        func(*conn);
    }
}

void ConnectionManager::check_timeouts(uint32_t idle_timeout_ms,
                                        uint32_t callback_timeout_ms,
                                        std::function<void(Connection&)> on_timeout) {
    std::vector<Connection*> timeout_conns;

    // 步骤1：锁内仅收集超时连接
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& pair : connections_) {
            Connection* conn = pair.second.get();
            bool is_idle_timeout = conn->is_timeout(idle_timeout_ms);
            bool is_cb_timeout = conn->is_callback_timeout(callback_timeout_ms);
            if (is_idle_timeout || is_cb_timeout) {
                timeout_conns.push_back(conn);
            }
        }
    }

    // 步骤2：锁外调用用户回调
    // 注意：回调中不应立即销毁Connection，应先标记关闭再异步清理
    // 指针有效期保证：即使回调中调用remove_connection，Connection对象在回调期间仍然有效
    // （因为unique_ptr只有在从map中erase后才会销毁对象）
    for (Connection* conn : timeout_conns) {
        on_timeout(*conn);
    }
}

void ConnectionManager::clear_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    connections_.clear();
}

void ConnectionManager::close_all() {
    // 与clear_all功能相同，用于兼容设计文档
    clear_all();
}

void ConnectionManager::get_statistics(utils::Statistics* stats) const {
    if (stats == nullptr) {
        return;
    }
    // 使用StatisticsManager获取统计信息
    utils::StatisticsManager::instance().get_statistics(stats);
}

} // namespace https_server_sim

// 文件结束
