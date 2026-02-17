#include "server/server.hpp"
#include "utils/logger.hpp"
#include <unistd.h>

namespace https_server_sim {
namespace server {

Server::Server()
    : state_(ServerState::STOPPED)
    , conn_manager_(std::make_unique<connection::ConnectionManager>())
    , callback_manager_(std::make_unique<callback::CallbackManager>())
    , running_(false)
{
}

Server::~Server() {
    stop();
    wait();
}

bool Server::load_config(const std::string& config_path) {
    return config_.load_from_file(config_path);
}

void Server::set_config(const config::Config& config) {
    config_ = config;
}

const config::Config& Server::get_config() const {
    return config_;
}

bool Server::start() {
    ServerState expected = ServerState::STOPPED;
    if (!state_.compare_exchange_strong(expected, ServerState::STARTING)) {
        return false;
    }

    LOG_INFO("Server", "Starting server...");

    // 初始化监听
    if (!init_listeners()) {
        state_ = ServerState::STOPPED;
        return false;
    }

    running_ = true;
    state_ = ServerState::RUNNING;

    // 启动工作线程
    worker_thread_ = std::thread(&Server::worker_thread, this);

    LOG_INFO("Server", "Server started");
    return true;
}

void Server::stop() {
    ServerState expected = ServerState::RUNNING;
    if (!state_.compare_exchange_strong(expected, ServerState::STOPPING)) {
        // 已经在停止或已停止
        expected = ServerState::STARTING;
        if (!state_.compare_exchange_strong(expected, ServerState::STOPPING)) {
            return;
        }
    }

    LOG_INFO("Server", "Stopping server...");
    running_ = false;
}

void Server::wait() {
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    cleanup();
    state_ = ServerState::STOPPED;
}

ServerState Server::get_state() const {
    return state_.load();
}

uint64_t Server::get_connection_count() const {
    return conn_manager_ ? conn_manager_->get_connection_count() : 0;
}

callback::CallbackManager& Server::get_callback_manager() {
    return *callback_manager_;
}

void Server::worker_thread() {
    LOG_INFO("Server", "Worker thread started");

    while (running_) {
        // 简单实现：休眠
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 检查超时
        if (conn_manager_) {
            conn_manager_->check_timeouts(30000, 30000,
                [this](connection::Connection& conn) {
                    LOG_WARN("Server", "Connection %lu timeout, closing", conn.get_id());
                    conn.close();
                    conn_manager_->remove_connection(conn.get_id());
                });
        }
    }

    LOG_INFO("Server", "Worker thread stopped");
}

bool Server::init_listeners() {
    // 简单实现：暂不实际创建socket
    const auto& listens = config_.get_listens();
    for (const auto& listen : listens) {
        if (listen.enabled) {
            LOG_INFO("Server", "Would listen on %s:%d", listen.ip.c_str(), listen.port);
            // 实际实现需要创建socket并监听
        }
    }
    return true;
}

void Server::cleanup() {
    // 清理监听socket
    for (int fd : listen_fds_) {
        if (fd >= 0) {
            ::close(fd);
        }
    }
    listen_fds_.clear();

    // 清理连接
    if (conn_manager_) {
        conn_manager_->clear_all();
    }
}

} // namespace server
} // namespace https_server_sim
