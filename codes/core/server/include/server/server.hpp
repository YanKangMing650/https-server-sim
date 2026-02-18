// =============================================================================
//  HTTPS Server Simulator - Server Module
//  文件: server.hpp
//  描述: Server类定义
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <chrono>
#include "config/config.hpp"
#include "connection/connection_manager.hpp"
#include "msg_center/msg_center.hpp"
#include "utils/statistics.hpp"

namespace https_server_sim {
namespace server {

// Server状态枚举
typedef enum {
    SERVER_STATUS_STOPPED = 0,
    SERVER_STATUS_INITIALIZING = 1,
    SERVER_STATUS_RUNNING = 2,
    SERVER_STATUS_SHUTTING_DOWN = 3,
    SERVER_STATUS_ERROR = 4
} ServerStatusEnum;

// Server状态结构体
struct ServerStatus {
    ServerStatusEnum status;
    uint64_t uptime_seconds;
    uint32_t current_connections;
    uint16_t listen_port;
    char listen_ip[64];
};

// Server类
class Server {
public:
    /**
     * @brief 构造函数
     */
    Server();

    /**
     * @brief 析构函数
     */
    ~Server();

    // 禁止拷贝
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    // ==================== 生命周期管理 ====================

    /**
     * @brief 初始化Server
     * @param config_file 配置文件路径
     * @return 0 成功，非0 失败
     */
    int init(const std::string& config_file);

    /**
     * @brief 启动Server
     * @return 0 成功，非0 失败
     */
    int start();

    /**
     * @brief 停止Server
     * @return 0 成功，非0 失败
     */
    int stop();

    /**
     * @brief 清理资源
     */
    void cleanup();

    // ==================== 状态查询 ====================

    /**
     * @brief 获取Server状态
     * @param status 输出参数，状态结构体指针
     */
    void get_status(ServerStatus* status) const;

    /**
     * @brief 获取统计信息
     * @param stats 输出参数，统计信息结构体指针
     */
    void get_statistics(utils::Statistics* stats) const;

private:
    // ==================== 私有方法 ====================

    /**
     * @brief 初始化监听socket
     * @return 0 成功，非0 失败
     */
    int init_listen_sockets();

    /**
     * @brief 执行优雅关闭
     */
    void graceful_shutdown();

    /**
     * @brief 停止接受新连接
     */
    void stop_accepting();

    /**
     * @brief 等待现有请求处理完成
     */
    void wait_pending_requests();

    /**
     * @brief 关闭所有连接
     */
    void close_all_connections();

    /**
     * @brief 清理资源
     */
    void cleanup_resources();

    /**
     * @brief 回滚监听socket（初始化失败时调用）
     */
    void rollback_listen_sockets();

    // ==================== 成员变量 ====================

    std::unique_ptr<config::Config> config_;
    std::unique_ptr<ConnectionManager> conn_manager_;
    std::unique_ptr<MsgCenter> msg_center_;

    std::vector<int> listen_fds_;
    std::vector<uint16_t> listen_ports_;
    std::vector<std::string> listen_ips_;

    ServerStatusEnum status_;
    std::atomic<bool> running_;
    std::atomic<bool> graceful_shutdown_;

    std::chrono::steady_clock::time_point start_time_;

    mutable std::mutex mutex_;

    // 超时常量
    static constexpr int MAX_CALLBACK_TIMEOUT_SECONDS = 30;
    static constexpr int MAX_CONN_CLOSE_WAIT_SECONDS = 5;
    static constexpr int DEFAULT_BACKLOG = 128;
};

} // namespace server
} // namespace https_server_sim

// 文件结束
