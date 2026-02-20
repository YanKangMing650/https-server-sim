// =============================================================================
//  HTTPS Server Simulator - MsgCenter Module
//  文件: event_queue.hpp
//  描述: EventQueue事件优先级队列类定义
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include "msg_center/event.hpp"
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace https_server_sim {

class EventQueue {
public:
    /**
     * @brief 构造函数
     * @param max_size 队列最大容量，默认10000
     */
    explicit EventQueue(size_t max_size = 10000);

    /**
     * @brief 析构函数
     */
    ~EventQueue();

    // 禁止拷贝
    EventQueue(const EventQueue&) = delete;
    EventQueue& operator=(const EventQueue&) = delete;

    /**
     * @brief 事件入队
     * @param event 要入队的事件
     * @return true-入队成功，false-队列已满
     */
    bool push(const Event& event);

    /**
     * @brief 事件出队（阻塞）
     * @return 出队的事件，队列为空且队列已关闭时返回type=SHUTDOWN的事件
     */
    Event pop();

    /**
     * @brief 事件出队（非阻塞）
     * @param event [out] 出队的事件
     * @return true-出队成功，false-队列空
     */
    bool try_pop(Event& event);

    /**
     * @brief 批量出队
     * @param max_count 最多出队的事件数量
     * @return 出队的事件vector
     */
    std::vector<Event> pop_all(size_t max_count);

    /**
     * @brief 队列是否为空
     * @return true-队列为空，false-队列非空
     */
    bool empty() const;

    /**
     * @brief 获取队列大小
     * @return 当前队列中的事件数量
     */
    size_t size() const;

    /**
     * @brief 队列是否已关闭
     * @return true-队列已关闭，false-队列未关闭
     */
    bool is_closed() const;

    /**
     * @brief 唤醒所有等待线程（用于停止时调用，会关闭队列）
     */
    void wake_up();

    /**
     * @brief 关闭队列（停止接受新事件，唤醒等待线程）
     */
    void close();

private:
    /**
     * @brief 获取最高优先级的非空队列索引
     * @return 队列索引，-1表示所有队列为空
     */
    int get_highest_priority_queue() const;

    // 按优先级分层的队列数组，索引0为最高优先级
    std::vector<std::queue<Event>> priority_queues_;

    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
    size_t max_size_;
    size_t size_;           // 队列当前大小（在mutex_保护下访问）
    std::atomic<bool> queue_closed_;
};

} // namespace https_server_sim

// 文件结束
