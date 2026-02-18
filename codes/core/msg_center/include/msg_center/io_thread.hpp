// =============================================================================
//  HTTPS Server Simulator - MsgCenter Module
//  文件: io_thread.hpp
//  描述: IoThread IO线程类定义
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include "msg_center/event.hpp"
#include "msg_center/event_queue.hpp"
#include <thread>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace https_server_sim {

class IoThread {
public:
    /**
     * @brief 构造函数
     * @param thread_id 线程ID
     * @param event_queue EventQueue指针，用于投递事件
     */
    IoThread(int thread_id, EventQueue* event_queue);

    /**
     * @brief 析构函数
     */
    ~IoThread();

    // 禁止拷贝
    IoThread(const IoThread&) = delete;
    IoThread& operator=(const IoThread&) = delete;

    /**
     * @brief 启动IO线程
     */
    void start();

    /**
     * @brief 停止IO线程
     */
    void stop();

    /**
     * @brief 添加监听socket
     * @note 【线程安全】此方法可从任意线程调用，内部使用fd_mutex_保护
     * @param fd socket文件描述符
     * @param port 端口号
     */
    void add_listen_fd(int fd, uint16_t port);

    /**
     * @brief 添加连接socket
     * @note 【线程安全】此方法可从任意线程调用，内部使用fd_mutex_保护
     * @param fd socket文件描述符
     * @param conn_id 连接ID
     */
    void add_conn_fd(int fd, uint64_t conn_id);

    /**
     * @brief 移除socket
     * @note 【线程安全】此方法可从任意线程调用，内部使用fd_mutex_保护
     * @param fd socket文件描述符
     */
    void remove_fd(int fd);

private:
    /**
     * @brief IO线程主函数
     */
    void io_thread_func();

    /**
     * @brief 平台特定的事件循环 - Linux
     */
    void event_loop_linux();

    /**
     * @brief 平台特定的事件循环 - Mac
     */
    void event_loop_mac();

    /**
     * @brief 平台特定的事件循环 - Windows
     */
    void event_loop_windows();

    /**
     * @brief 唤醒IO线程（用于添加/移除fd时唤醒）
     */
    void wake_up();

    int thread_id_;
    EventQueue* event_queue_;
    std::thread thread_;
    std::atomic<bool> running_;

    // 平台特定的事件循环fd
    int epoll_fd_;     // Linux: epoll fd
    int kq_fd_;        // Mac: kqueue fd
    void* iocp_handle_; // Windows: IOCP handle

    // 唤醒fd
    int wakeup_fd_;    // Linux: eventfd, Mac: pipe write fd

    // fd到conn_id的映射
    std::unordered_map<int, uint64_t> fd_to_conn_id_;

    // 监听fd到端口的映射
    std::unordered_map<int, uint16_t> listen_fd_to_port_;

    // 监听fd列表（用于快速遍历，数据冗余设计）
    // 保留vector是为了在某些场景下需要快速遍历所有监听fd
    std::vector<int> listen_fds_;

    // 连接fd列表（用于快速遍历，数据冗余设计）
    // 保留vector是为了在某些场景下需要快速遍历所有连接fd
    std::vector<int> conn_fds_;

    // 保护fd管理的锁
    mutable std::mutex fd_mutex_;
};

} // namespace https_server_sim

// 文件结束
