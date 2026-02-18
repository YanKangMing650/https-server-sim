// =============================================================================
//  HTTPS Server Simulator - MsgCenter Module
//  文件: io_thread.cpp
//  描述: IoThread IO线程类实现
//  版权: Copyright (c) 2026
// =============================================================================
#include "msg_center/io_thread.hpp"
#include <chrono>
#include <algorithm>

namespace https_server_sim {

IoThread::IoThread(int thread_id, EventQueue* event_queue)
    : thread_id_(thread_id)
    , event_queue_(event_queue)
    , running_(false)
    , epoll_fd_(-1)
    , kq_fd_(-1)
    , iocp_handle_(nullptr)
    , wakeup_fd_(-1)
{
    // 标记当前未使用的变量（为未来完整实现预留）
    (void)thread_id_;
    (void)event_queue_;
    (void)epoll_fd_;
    (void)kq_fd_;
    (void)iocp_handle_;
    (void)wakeup_fd_;
}

IoThread::~IoThread() {
    stop();
}

void IoThread::start() {
    if (running_.load(std::memory_order_acquire)) {
        return;
    }

    running_.store(true, std::memory_order_release);
    thread_ = std::thread(&IoThread::io_thread_func, this);
}

void IoThread::stop() {
    if (!running_.load(std::memory_order_acquire)) {
        return;
    }

    running_.store(false, std::memory_order_release);
    wake_up();

    if (thread_.joinable()) {
        thread_.join();
    }
}

void IoThread::add_listen_fd(int fd, uint16_t port) {
    std::lock_guard<std::mutex> lock(fd_mutex_);

    listen_fd_to_port_[fd] = port;
    // 同步添加到listen_fds_（避免重复）
    auto it = std::find(listen_fds_.begin(), listen_fds_.end(), fd);
    if (it == listen_fds_.end()) {
        listen_fds_.push_back(fd);
    }

    wake_up();
}

void IoThread::add_conn_fd(int fd, uint64_t conn_id) {
    std::lock_guard<std::mutex> lock(fd_mutex_);

    fd_to_conn_id_[fd] = conn_id;
    // 同步添加到conn_fds_（避免重复）
    auto it = std::find(conn_fds_.begin(), conn_fds_.end(), fd);
    if (it == conn_fds_.end()) {
        conn_fds_.push_back(fd);
    }

    wake_up();
}

void IoThread::remove_fd(int fd) {
    std::lock_guard<std::mutex> lock(fd_mutex_);

    // 从listen数据结构中移除
    auto listen_it = listen_fd_to_port_.find(fd);
    if (listen_it != listen_fd_to_port_.end()) {
        listen_fd_to_port_.erase(listen_it);
        // 同步从listen_fds_中移除
        auto vec_it = std::find(listen_fds_.begin(), listen_fds_.end(), fd);
        if (vec_it != listen_fds_.end()) {
            listen_fds_.erase(vec_it);
        }
    }

    // 从conn数据结构中移除
    auto conn_it = fd_to_conn_id_.find(fd);
    if (conn_it != fd_to_conn_id_.end()) {
        fd_to_conn_id_.erase(conn_it);
        // 同步从conn_fds_中移除
        auto vec_it = std::find(conn_fds_.begin(), conn_fds_.end(), fd);
        if (vec_it != conn_fds_.end()) {
            conn_fds_.erase(vec_it);
        }
    }

    wake_up();
}

void IoThread::io_thread_func() {
    // 简化实现：桩代码，避免复杂的平台特定逻辑
    // 详细设计文档中的完整实现需要处理epoll/kqueue/IOCP
    while (running_.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void IoThread::event_loop_linux() {
    // 桩代码：按详细设计文档需要完整实现epoll逻辑
    // 这里仅作为占位符
    while (running_.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void IoThread::event_loop_mac() {
    // 桩代码：按详细设计文档需要完整实现kqueue逻辑
    // 这里仅作为占位符
    while (running_.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void IoThread::event_loop_windows() {
    // 桩代码：按详细设计文档需要完整实现IOCP逻辑
    // 这里仅作为占位符
    while (running_.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void IoThread::wake_up() {
    // 桩代码：按详细设计文档需要实现平台特定的唤醒机制
    // 这里仅作为占位符
}

} // namespace https_server_sim

// 文件结束
