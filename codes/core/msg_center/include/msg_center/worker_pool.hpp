// =============================================================================
//  HTTPS Server Simulator - MsgCenter Module
//  文件: worker_pool.hpp
//  描述: WorkerPool工作线程池类定义
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include "msg_center/event_loop.hpp"
#include <vector>
#include <thread>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace https_server_sim {

class WorkerPool {
public:
    /**
     * @brief 构造函数
     * @param num_workers 工作线程数量，默认2
     * @param event_loop 可选的EventLoop指针，用于投递CALLBACK_DONE事件
     */
    WorkerPool(size_t num_workers = 2, EventLoop* event_loop = nullptr);

    /**
     * @brief 析构函数
     */
    ~WorkerPool();

    // 禁止拷贝
    WorkerPool(const WorkerPool&) = delete;
    WorkerPool& operator=(const WorkerPool&) = delete;

    /**
     * @brief 启动线程池
     */
    void start();

    /**
     * @brief 停止线程池
     */
    void stop();

    /**
     * @brief 投递任务
     * @param task 要执行的任务函数
     */
    void post_task(std::function<void()> task);

    /**
     * @brief 获取线程数
     * @return 工作线程数量
     */
    size_t get_thread_count() const;

    /**
     * @brief 设置是否投递CALLBACK_DONE事件
     * @param enable true-投递，false-不投递
     */
    void set_post_callback_done(bool enable);

private:
    /**
     * @brief 工作线程函数
     */
    void worker_thread();

    /**
     * @brief 投递回调完成事件
     */
    void post_callback_done_event();

    size_t num_workers_;
    std::vector<std::thread> workers_;

    // 任务队列：std::queue + mutex（MPMC线程安全）
    std::queue<std::function<void()>> task_queue_;
    mutable std::mutex mutex_;
    std::condition_variable task_cv_;

    std::atomic<bool> running_;
    std::atomic<bool> queue_closed_;

    // 关联的EventLoop（可选，用于投递CALLBACK_DONE事件）
    EventLoop* event_loop_;

    // 是否投递CALLBACK_DONE事件
    std::atomic<bool> post_callback_done_;
};

} // namespace https_server_sim

// 文件结束
