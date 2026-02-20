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

std::shared_ptr<Connection> ConnectionManager::create_connection(int fd, uint16_t server_port) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t id = next_connection_id_++;
    auto conn = std::make_shared<Connection>(id, fd, server_port, time_source_.get());
    connections_[id] = conn;
    return conn;
}

std::shared_ptr<Connection> ConnectionManager::get_connection(uint64_t conn_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = connections_.find(conn_id);
    return (it != connections_.end()) ? it->second : nullptr;
}

std::shared_ptr<const Connection> ConnectionManager::get_connection(uint64_t conn_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = connections_.find(conn_id);
    return (it != connections_.end()) ? it->second : nullptr;
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
    std::vector<std::shared_ptr<Connection>> conns;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        conns.reserve(connections_.size());
        for (auto& pair : connections_) {
            conns.push_back(pair.second);
        }
    }
    for (auto& conn : conns) {
        if (conn) {
            func(*conn);
        }
    }
}

void ConnectionManager::for_each_connection(std::function<void(const Connection&)> func) const {
    std::vector<std::shared_ptr<const Connection>> conns;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        conns.reserve(connections_.size());
        for (const auto& pair : connections_) {
            conns.push_back(pair.second);
        }
    }
    for (const auto& conn : conns) {
        if (conn) {
            func(*conn);
        }
    }
}

void ConnectionManager::check_timeouts(uint32_t idle_timeout_ms,
                                        uint32_t callback_timeout_ms,
                                        std::function<void(Connection&)> on_timeout) {
    std::vector<std::shared_ptr<Connection>> timeout_conns;

    // 步骤1：锁内仅收集超时连接（使用shared_ptr保证安全）
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& pair : connections_) {
            auto& conn = pair.second;
            bool is_idle_timeout = conn->is_timeout(idle_timeout_ms);
            bool is_cb_timeout = conn->is_callback_timeout(callback_timeout_ms);
            if (is_idle_timeout || is_cb_timeout) {
                timeout_conns.push_back(conn);
            }
        }
    }

    // 步骤2：锁外调用用户回调
    // shared_ptr保证即使remove_connection被调用，对象仍然安全
    for (auto& conn : timeout_conns) {
        if (conn) {
            on_timeout(*conn);
        }
    }
}

void ConnectionManager::check_callback_timeouts(uint32_t timeout_seconds) {
    std::vector<std::shared_ptr<Connection>> timeout_conns;

    // 转换为毫秒
    uint32_t timeout_ms = timeout_seconds * 1000;

    // 步骤1：锁内仅收集回调超时连接（使用shared_ptr保证安全）
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& pair : connections_) {
            auto& conn = pair.second;
            if (conn->is_callback_timeout(timeout_ms)) {
                timeout_conns.push_back(conn);
            }
        }
    }

    // 步骤2：锁外关闭超时连接
    for (auto& conn : timeout_conns) {
        if (conn) {
            conn->close();
        }
    }
}

void ConnectionManager::clear_all() {
    // clear_all已弃用，调用close_all()保持功能一致
    close_all();
}

void ConnectionManager::close_all() {
    std::vector<std::shared_ptr<Connection>> conns_to_close;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // 先把所有连接转移到临时vector（锁外关闭）
        for (auto& pair : connections_) {
            conns_to_close.push_back(pair.second);
        }
        connections_.clear();
    }
    // 锁外调用每个Connection的close()
    for (auto& conn : conns_to_close) {
        if (conn) {
            conn->close();
        }
    }
    // conns_to_close离开作用域时自动释放所有Connection
}

void ConnectionManager::get_statistics(utils::Statistics* stats) const {
    if (stats == nullptr) {
        return;
    }
    // 直接从StatisticsManager获取完整统计信息
    utils::StatisticsManager::instance().get_statistics(stats);
}

} // namespace https_server_sim

// 文件结束
