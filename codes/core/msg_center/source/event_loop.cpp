// =============================================================================
//  HTTPS Server Simulator - MsgCenter Module
//  文件: event_loop.cpp
//  描述: EventLoop事件主循环类实现
//  版权: Copyright (c) 2026
// =============================================================================
#include "msg_center/event_loop.hpp"
#include <chrono>

namespace https_server_sim {

EventLoop::EventLoop(EventQueue* event_queue)
    : event_queue_(event_queue)
    , running_(false)
    , started_(false)
{}

EventLoop::~EventLoop() = default;

void EventLoop::run() {
    loop_thread_id_ = std::this_thread::get_id();
    running_.store(true, std::memory_order_release);

    // 通知等待的线程：EventLoop已启动
    {
        std::lock_guard<std::mutex> lock(start_mutex_);
        started_.store(true, std::memory_order_release);
    }
    start_cv_.notify_all();

    while (running_.load(std::memory_order_acquire)) {
        Event event;
        if (event_queue_->try_pop(event)) {
            if (event.type != EventType::SHUTDOWN) {
                if (event.handler) {
                    event.handler();
                }
            } else {
                // 收到SHUTDOWN事件，退出循环
                break;
            }
        } else {
            // 队列为空，短暂休眠后继续检查running_标志
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    running_.store(false, std::memory_order_release);
}

void EventLoop::stop() {
    running_.store(false, std::memory_order_release);

    if (event_queue_) {
        // 尝试推送SHUTDOWN事件作为补充保障，但不关闭队列
        (void)event_queue_->push(Event::make_shutdown_event());
    }
}

void EventLoop::post_event(const Event& event) {
    if (event_queue_) {
        event_queue_->push(event);
    }
}

bool EventLoop::is_in_loop_thread() const {
    return std::this_thread::get_id() == loop_thread_id_;
}

bool EventLoop::wait_for_started(int timeout_ms) {
    std::unique_lock<std::mutex> lock(start_mutex_);
    return start_cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]() {
        return started_.load(std::memory_order_acquire);
    });
}

} // namespace https_server_sim

// 文件结束
