// =============================================================================
//  HTTPS Server Simulator - MsgCenter Module
//  文件: msg_center.hpp
//  描述: MsgCenter模块门面类定义
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include "msg_center/event.hpp"
#include "msg_center/event_queue.hpp"
#include "msg_center/event_loop.hpp"
#include "msg_center/worker_pool.hpp"
#include "msg_center/io_thread.hpp"
#include "utils/statistics.hpp"
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>

namespace https_server_sim {

class MsgCenter {
public:
    /**
     * @brief 构造函数
     * @param io_thread_count IO线程数量，默认2
     * @param worker_thread_count 工作线程数量，默认2
     */
    explicit MsgCenter(size_t io_thread_count = 2, size_t worker_thread_count = 2);

    /**
     * @brief 析构函数
     */
    ~MsgCenter();

    // 禁止拷贝
    MsgCenter(const MsgCenter&) = delete;
    MsgCenter& operator=(const MsgCenter&) = delete;

    /**
     * @brief 启动消息中心
     * @return 0 (MsgCenterError::SUCCESS) 表示成功，非0表示错误码
     */
    int start();

    /**
     * @brief 停止消息中心
     */
    void stop();

    /**
     * @brief 提交事件
     * @param event 要提交的事件
     */
    void post_event(const Event& event);

    /**
     * @brief 提交回调任务
     * @param task 要执行的回调任务
     */
    void post_callback_task(std::function<void()> task);

    /**
     * @brief 获取EventLoop
     * @return EventLoop指针
     */
    EventLoop* get_event_loop() { return event_loop_.get(); }

    /**
     * @brief 添加监听socket到消息中心
     * @param fd 监听socket文件描述符
     * @return 0 表示成功，非0 表示错误码
     */
    int add_listen_fd(int fd);

    /**
     * @brief 从消息中心移除监听socket
     * @param fd 监听socket文件描述符
     * @return 0 表示成功，非0 表示错误码
     */
    int remove_listen_fd(int fd);

    /**
     * @brief 获取统计信息
     * @param stats 输出参数，统计信息结构体指针
     */
    void get_statistics(utils::Statistics* stats) const;

private:
    std::unique_ptr<EventQueue> event_queue_;
    std::unique_ptr<EventLoop> event_loop_;
    std::unique_ptr<WorkerPool> worker_pool_;
    std::vector<std::unique_ptr<IoThread>> io_threads_;
    std::thread event_loop_thread_;
    std::atomic<bool> running_;

    std::vector<int> listen_fds_;

    size_t io_thread_count_;
    size_t worker_thread_count_;
};

} // namespace https_server_sim

// 文件结束
