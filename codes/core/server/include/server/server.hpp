#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include "config/config.hpp"
#include "connection/connection.hpp"
#include "callback/callback_strategy.hpp"

namespace https_server_sim {
namespace server {

// Server状态
enum class ServerState : uint8_t {
    STOPPED,
    STARTING,
    RUNNING,
    STOPPING
};

// Server类
class Server {
public:
    Server();
    ~Server();

    // 禁止拷贝
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    // ========== 配置 ==========

    // 加载配置文件
    bool load_config(const std::string& config_path);

    // 设置配置
    void set_config(const config::Config& config);

    // 获取配置
    const config::Config& get_config() const;

    // ========== 生命周期 ==========

    // 启动服务器
    bool start();

    // 停止服务器
    void stop();

    // 等待服务器停止
    void wait();

    // 获取当前状态
    ServerState get_state() const;

    // ========== 统计 ==========

    // 获取当前连接数
    uint64_t get_connection_count() const;

    // ========== 回调管理 ==========

    // 获取回调管理器
    callback::CallbackManager& get_callback_manager();

private:
    // 工作线程函数
    void worker_thread();

    // 初始化监听socket
    bool init_listeners();

    // 清理资源
    void cleanup();

    config::Config config_;
    std::atomic<ServerState> state_;
    std::unique_ptr<connection::ConnectionManager> conn_manager_;
    std::unique_ptr<callback::CallbackManager> callback_manager_;
    std::vector<int> listen_fds_;
    std::thread worker_thread_;
    std::atomic<bool> running_;
};

} // namespace server
} // namespace https_server_sim
