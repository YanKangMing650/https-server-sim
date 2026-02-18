// =============================================================================
//  HTTPS Server Simulator - MsgCenter Module
//  文件: event_loop.hpp
//  描述: EventLoop事件主循环类定义
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include "msg_center/event.hpp"
#include "msg_center/event_queue.hpp"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace https_server_sim {

class EventLoop {
public:
    /**
     * @brief 构造函数
     * @param event_queue EventQueue指针，不可为nullptr
     */
    EventLoop(EventQueue* event_queue);

    /**
     * @brief 析构函数
     */
    ~EventLoop();

    // 禁止拷贝
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    /**
     * @brief 运行事件循环（阻塞当前线程）
     */
    void run();

    /**
     * @brief 停止事件循环
     */
    void stop();

    /**
     * @brief 投递事件
     * @param event 要投递的事件
     */
    void post_event(const Event& event);

    /**
     * @brief 检查是否在事件循环线程
     * @return true-当前是事件循环线程，false-不是
     */
    bool is_in_loop_thread() const;

    /**
     * @brief 等待EventLoop启动完成
     * @param timeout_ms 超时时间，毫秒
     * @return true-启动成功，false-超时
     */
    bool wait_for_started(int timeout_ms = 1000);

    /**
     * @brief 获取EventQueue指针
     * @return EventQueue指针
     */
    EventQueue* get_event_queue() { return event_queue_; }

private:
    EventQueue* event_queue_;
    std::atomic<bool> running_;
    std::atomic<bool> started_;
    std::thread::id loop_thread_id_;

    // 启动同步
    std::mutex start_mutex_;
    std::condition_variable start_cv_;
};

} // namespace https_server_sim

// 文件结束
