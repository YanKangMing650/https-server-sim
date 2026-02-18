// =============================================================================
//  HTTPS Server Simulator - Connection Module
//  文件: connection_manager.hpp
//  描述: ConnectionManager 类定义
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <functional>
#include "connection/connection.hpp"
#include "utils/statistics.hpp"

namespace https_server_sim {

// 连接管理器
class ConnectionManager {
public:
    ConnectionManager();
    explicit ConnectionManager(std::unique_ptr<TimeSource> time_source);
    ~ConnectionManager();

    // 禁止拷贝
    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;

    // 创建连接
    // fd: socket文件描述符
    // server_port: server监听端口
    // return: Connection指针（有效期至remove_connection调用前）
    Connection* create_connection(int fd, uint16_t server_port);

    // 获取连接
    Connection* get_connection(uint64_t conn_id);
    const Connection* get_connection(uint64_t conn_id) const;

    // 移除连接
    void remove_connection(uint64_t conn_id);

    // 获取连接数
    uint32_t get_connection_count() const;

    // 遍历所有连接
    void for_each_connection(std::function<void(Connection&)> func);

    // const 遍历所有连接（const重载）
    void for_each_connection(std::function<void(const Connection&)> func) const;

    // 检查超时（锁内收集，锁外回调）
    // 注意：回调中不应立即销毁Connection，应先标记关闭再异步清理
    // 指针有效期保证：回调期间Connection对象保持有效（即使调用remove_connection）
    void check_timeouts(uint32_t idle_timeout_ms,
                        uint32_t callback_timeout_ms,
                        std::function<void(Connection&)> on_timeout);

    // 清空所有连接
    void clear_all();

    // 关闭所有连接（与clear_all功能相同，用于兼容设计文档）
    void close_all();

    // 获取统计信息
    void get_statistics(utils::Statistics* stats) const;

private:
    uint64_t next_connection_id_;
    std::unordered_map<uint64_t, std::unique_ptr<Connection>> connections_;
    mutable std::mutex mutex_;
    std::unique_ptr<TimeSource> time_source_;
};

} // namespace https_server_sim

// 文件结束
