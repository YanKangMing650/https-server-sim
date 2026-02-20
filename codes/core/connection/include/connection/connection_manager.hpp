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
    // return: Connection的shared_ptr，可安全持有
    std::shared_ptr<Connection> create_connection(int fd, uint16_t server_port);

    // 获取连接
    std::shared_ptr<Connection> get_connection(uint64_t conn_id);
    std::shared_ptr<const Connection> get_connection(uint64_t conn_id) const;

    // 移除连接
    void remove_connection(uint64_t conn_id);

    // 获取连接数
    uint32_t get_connection_count() const;

    // 遍历所有连接
    void for_each_connection(std::function<void(Connection&)> func);

    // const 遍历所有连接（const重载）
    void for_each_connection(std::function<void(const Connection&)> func) const;

    // 检查超时（锁内收集，锁外回调）
    // 使用shared_ptr保证回调期间Connection对象安全
    void check_timeouts(uint32_t idle_timeout_ms,
                        uint32_t callback_timeout_ms,
                        std::function<void(Connection&)> on_timeout);

    // 检查回调超时（设计文档要求的接口）
    // timeout_seconds: 超时时间（秒）
    void check_callback_timeouts(uint32_t timeout_seconds);

    // 清空所有连接（已弃用，建议使用close_all()）
    // @deprecated 请使用 close_all() 代替
    void clear_all();

    // 关闭所有连接（推荐使用）
    void close_all();

    // 获取统计信息
    void get_statistics(utils::Statistics* stats) const;

private:
    uint64_t next_connection_id_;
    std::unordered_map<uint64_t, std::shared_ptr<Connection>> connections_;
    mutable std::mutex mutex_;
    std::unique_ptr<TimeSource> time_source_;
};

} // namespace https_server_sim

// 文件结束
