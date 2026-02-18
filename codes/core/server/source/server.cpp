// =============================================================================
//  HTTPS Server Simulator - Server Module
//  文件: server.cpp
//  描述: Server类实现
//  版权: Copyright (c) 2026
// =============================================================================
#include "server/server.hpp"
#include "utils/logger.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <thread>

namespace https_server_sim {
namespace server {

// 错误码定义
static constexpr int ERR_SUCCESS = 0;
static constexpr int ERR_INVALID_STATE = -1;
static constexpr int ERR_CONFIG_LOAD = -2;
static constexpr int ERR_CONFIG_VALIDATE = -3;
static constexpr int ERR_SOCKET_CREATE = -4;
static constexpr int ERR_SOCKET_BIND = -5;
static constexpr int ERR_SOCKET_LISTEN = -6;
static constexpr int ERR_MSG_CENTER_START = -7;
static constexpr int ERR_INVALID_ARGUMENT = -8;
static constexpr int ERR_INTERNAL = -9;

Server::Server()
    : status_(SERVER_STATUS_STOPPED)
    , running_(false)
    , graceful_shutdown_(false)
{
}

Server::~Server()
{
    cleanup();
}

int Server::init(const std::string& config_file)
{
    // 步骤1: 设置状态（仅在修改status_时加锁）
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (status_ != SERVER_STATUS_STOPPED) {
            return ERR_INVALID_STATE;
        }
        status_ = SERVER_STATUS_INITIALIZING;
    }

    try {
        // 步骤2: 创建并加载配置（不加锁）
        config_ = std::make_unique<config::Config>();
        int ret = config_->load_from_file(config_file);
        if (ret != ERR_SUCCESS) {
            cleanup();
            std::lock_guard<std::mutex> lock(mutex_);
            status_ = SERVER_STATUS_ERROR;
            return ERR_CONFIG_LOAD;
        }

        // 步骤3: 校验配置（不加锁）
        ret = config_->validate();
        if (ret != ERR_SUCCESS) {
            cleanup();
            std::lock_guard<std::mutex> lock(mutex_);
            status_ = SERVER_STATUS_ERROR;
            return ERR_CONFIG_VALIDATE;
        }

        // 步骤4: 初始化监听socket（不加锁）
        int socket_ret = init_listen_sockets();
        if (socket_ret != ERR_SUCCESS) {
            cleanup();
            std::lock_guard<std::mutex> lock(mutex_);
            status_ = SERVER_STATUS_ERROR;
            return socket_ret;
        }

        // 步骤5: 创建子模块（不加锁）
        conn_manager_ = std::make_unique<ConnectionManager>();
        msg_center_ = std::make_unique<MsgCenter>();

        // 步骤6: 设置状态（仅在修改status_时加锁）
        {
            std::lock_guard<std::mutex> lock(mutex_);
            status_ = SERVER_STATUS_STOPPED;
        }
        return ERR_SUCCESS;
    } catch (...) {
        cleanup();
        std::lock_guard<std::mutex> lock(mutex_);
        status_ = SERVER_STATUS_ERROR;
        return ERR_INTERNAL;
    }
}

int Server::start()
{
    // 步骤1: 检查当前状态并设置RUNNING（加锁）
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // 必须已init（有config）且状态为STOPPED
        if (!config_ || status_ != SERVER_STATUS_STOPPED) {
            return ERR_INVALID_STATE;
        }
        status_ = SERVER_STATUS_RUNNING;
        running_ = true;
    }

    // 步骤2: 启动MsgCenter
    int ret = msg_center_->start();
    if (ret != ERR_SUCCESS) {
        // MsgCenter启动失败，清理资源
        {
            std::lock_guard<std::mutex> lock(mutex_);
            status_ = SERVER_STATUS_ERROR;
            running_ = false;
        }
        cleanup();
        return ERR_MSG_CENTER_START;
    }

    // 步骤3: 注册监听socket到MsgCenter
    for (int fd : listen_fds_) {
        ret = msg_center_->add_listen_fd(fd);
        if (ret != ERR_SUCCESS) {
            // 注册失败，回滚已注册的fd
            stop_accepting();
            msg_center_->stop();
            {
                std::lock_guard<std::mutex> lock(mutex_);
                status_ = SERVER_STATUS_ERROR;
                running_ = false;
            }
            cleanup();
            return ERR_INTERNAL;
        }
    }

    // 步骤4: 记录启动时间（不加锁）
    start_time_ = std::chrono::steady_clock::now();

    LOG_INFO("Server", "Server started successfully");
    return ERR_SUCCESS;
}

int Server::stop()
{
    // 步骤1: 检查当前状态（加锁）
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (status_ != SERVER_STATUS_RUNNING) {
            return ERR_INVALID_STATE;
        }
    }

    // 步骤2: 执行优雅关闭（不加锁）
    graceful_shutdown();

    LOG_INFO("Server", "Server stopped successfully");
    return ERR_SUCCESS;
}

void Server::cleanup()
{
    // 步骤1: 清理监听socket（不加锁，避免耗时操作阻塞其他线程）
    for (int fd : listen_fds_) {
        if (fd >= 0) {
            ::close(fd);
        }
    }
    listen_fds_.clear();
    listen_ports_.clear();
    listen_ips_.clear();

    // 步骤2: 销毁子模块（不加锁）
    msg_center_.reset();
    conn_manager_.reset();

    // 步骤3: 清理配置（不加锁）
    config_.reset();

    // 步骤4: 重置标志（不加锁，原子变量）
    running_ = false;
    graceful_shutdown_ = false;

    // 步骤5: 恢复状态为STOPPED（仅在修改status_时加锁）
    {
        std::lock_guard<std::mutex> lock(mutex_);
        status_ = SERVER_STATUS_STOPPED;
    }
}

void Server::get_status(ServerStatus* status) const
{
    if (status == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    status->status = status_;

    // 计算运行时间
    if (status_ == SERVER_STATUS_RUNNING &&
        start_time_ != std::chrono::steady_clock::time_point()) {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
        status->uptime_seconds = static_cast<uint64_t>(duration.count());
    } else {
        status->uptime_seconds = 0;
    }

    // 获取当前连接数
    if (conn_manager_) {
        status->current_connections = conn_manager_->get_connection_count();
    } else {
        status->current_connections = 0;
    }

    // 获取第一个监听端口和IP
    if (!listen_ports_.empty()) {
        status->listen_port = listen_ports_[0];
    } else {
        status->listen_port = 0;
    }

    // 清空并复制监听IP
    std::memset(status->listen_ip, 0, sizeof(status->listen_ip));
    if (!listen_ips_.empty()) {
        size_t copy_len = std::min(listen_ips_[0].length(), sizeof(status->listen_ip) - 1);
        if (copy_len > 0) {
            std::strncpy(status->listen_ip, listen_ips_[0].c_str(), copy_len);
            status->listen_ip[copy_len] = '\0';
        }
    }
}

void Server::get_statistics(utils::Statistics* stats) const
{
    if (stats == nullptr) {
        return;
    }

    // 从ConnectionManager获取统计信息
    utils::Statistics conn_stats;
    if (conn_manager_) {
        conn_manager_->get_statistics(&conn_stats);
    }

    // 从MsgCenter获取统计信息
    utils::Statistics msg_stats;
    if (msg_center_) {
        msg_center_->get_statistics(&msg_stats);
    }

    // 聚合统计信息
    // 注意：当前实现中，ConnectionManager和MsgCenter都使用StatisticsManager，
    // 所以直接获取StatisticsManager的统计信息即可
    utils::StatisticsManager::instance().get_statistics(stats);
}

int Server::init_listen_sockets()
{
    int ret;
    const auto& listens = config_->get_listens();

    for (const auto& listen_cfg : listens) {
        if (!listen_cfg.enabled) {
            continue;
        }

        // 创建socket
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            LOG_ERROR("Server", "Failed to create socket");
            rollback_listen_sockets();
            return ERR_SOCKET_CREATE;
        }

        // 设置SO_REUSEADDR选项
        int opt = 1;
        ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        if (ret < 0) {
            LOG_ERROR("Server", "Failed to set SO_REUSEADDR");
            ::close(fd);
            rollback_listen_sockets();
            return ERR_INTERNAL;
        }

        // 填充sockaddr_in结构
        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(listen_cfg.port);

        // 检查inet_pton返回值
        ret = inet_pton(AF_INET, listen_cfg.ip.c_str(), &addr.sin_addr);
        if (ret <= 0) {
            LOG_ERROR("Server", "Failed to convert IP address: %s", listen_cfg.ip.c_str());
            ::close(fd);
            rollback_listen_sockets();
            return ERR_INVALID_ARGUMENT;
        }

        // 绑定
        ret = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
        if (ret < 0) {
            LOG_ERROR("Server", "Failed to bind to %s:%d", listen_cfg.ip.c_str(), listen_cfg.port);
            ::close(fd);
            rollback_listen_sockets();
            return ERR_SOCKET_BIND;
        }

        // 监听 - 使用配置中的backlog或默认值
        uint32_t backlog = listen_cfg.backlog > 0 ? listen_cfg.backlog : DEFAULT_BACKLOG;
        ret = listen(fd, static_cast<int>(backlog));
        if (ret < 0) {
            LOG_ERROR("Server", "Failed to listen on %s:%d", listen_cfg.ip.c_str(), listen_cfg.port);
            ::close(fd);
            rollback_listen_sockets();
            return ERR_SOCKET_LISTEN;
        }

        // 加入列表
        listen_fds_.push_back(fd);
        listen_ports_.push_back(listen_cfg.port);
        listen_ips_.push_back(listen_cfg.ip);

        LOG_INFO("Server", "Listening on %s:%d", listen_cfg.ip.c_str(), listen_cfg.port);
    }

    return ERR_SUCCESS;
}

void Server::graceful_shutdown()
{
    // 步骤1: 设置标志（无锁，原子变量）和状态（加锁保护）
    graceful_shutdown_ = true;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        status_ = SERVER_STATUS_SHUTTING_DOWN;
    }

    // 步骤2: 停止接受新连接（不加锁）
    stop_accepting();

    // 步骤3: 等待现有请求（最多30秒，不加锁）
    wait_pending_requests();

    // 步骤4: 关闭连接（最多5秒等待，不加锁）
    close_all_connections();

    // 步骤5: 清理资源（不加锁，避免耗时操作在锁内）
    cleanup_resources();

    // 步骤6: 设置最终状态（加锁保护）
    {
        std::lock_guard<std::mutex> lock(mutex_);
        status_ = SERVER_STATUS_STOPPED;
    }
}

void Server::stop_accepting()
{
    for (size_t i = 0; i < listen_fds_.size(); ++i) {
        int fd = listen_fds_[i];
        // 先从MsgCenter移除fd
        if (msg_center_) {
            msg_center_->remove_listen_fd(fd);
        }
        // 再关闭fd
        if (fd >= 0) {
            ::close(fd);
        }
    }
}

void Server::wait_pending_requests()
{
    auto start = std::chrono::steady_clock::now();
    while (true) {
        // 注意：设计文档要求调用check_callback_timeouts，
        // 但ConnectionManager实际提供的是check_timeouts接口
        if (conn_manager_) {
            conn_manager_->check_timeouts(
                MAX_CALLBACK_TIMEOUT_SECONDS * 1000,
                MAX_CALLBACK_TIMEOUT_SECONDS * 1000,
                [](Connection& conn) {
                    conn.close();
                });
        }

        // 判断所有请求处理完成
        if (!conn_manager_ || conn_manager_->get_connection_count() == 0) {
            break;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= MAX_CALLBACK_TIMEOUT_SECONDS) {
            LOG_WARN("Server", "Wait pending requests timeout after %d seconds",
                     MAX_CALLBACK_TIMEOUT_SECONDS);
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Server::close_all_connections()
{
    // 使用设计文档要求的close_all()接口
    if (conn_manager_) {
        conn_manager_->close_all();
    }

    // 等待连接完全关闭
    auto start = std::chrono::steady_clock::now();
    while (true) {
        if (!conn_manager_ || conn_manager_->get_connection_count() == 0) {
            break;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= MAX_CONN_CLOSE_WAIT_SECONDS) {
            LOG_WARN("Server", "Close all connections timeout after %d seconds",
                     MAX_CONN_CLOSE_WAIT_SECONDS);
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Server::cleanup_resources()
{
    // 停止MsgCenter
    if (msg_center_) {
        msg_center_->stop();
    }

    // 清空监听socket列表（socket已在stop_accepting中关闭）
    listen_fds_.clear();
    listen_ports_.clear();
    listen_ips_.clear();

    // 设置运行标志为false
    running_ = false;
}

void Server::rollback_listen_sockets()
{
    // 关闭所有已创建的socket
    for (int fd : listen_fds_) {
        if (fd >= 0) {
            ::close(fd);
        }
    }
    // 清空列表
    listen_fds_.clear();
    listen_ports_.clear();
    listen_ips_.clear();
}

} // namespace server
} // namespace https_server_sim

// 文件结束
