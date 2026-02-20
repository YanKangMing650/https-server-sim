// =============================================================================
//  HTTPS Server Simulator - MsgCenter Module
//  文件: event_queue.cpp
//  描述: EventQueue事件优先级队列类实现
//  版权: Copyright (c) 2026
// =============================================================================
#include "msg_center/event_queue.hpp"

namespace https_server_sim {

EventQueue::EventQueue(size_t max_size)
    : max_size_(max_size)
    , size_(0)
    , queue_closed_(false)
{
    // 使用kEventPriorityCount常量，避免硬编码
    priority_queues_.resize(kEventPriorityCount);
}

EventQueue::~EventQueue() = default;

bool EventQueue::push(const Event& event) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 使用size_检查队列满（在锁保护下访问）
    if (size_ >= max_size_) {
        return false;
    }

    // 校验event.type的取值范围，防止越界访问
    size_t priority = static_cast<size_t>(event.type);
    if (priority >= kEventPriorityCount) {
        return false;
    }

    priority_queues_[priority].push(event);
    ++size_;

    not_empty_.notify_one();
    return true;
}

Event EventQueue::pop() {
    std::unique_lock<std::mutex> lock(mutex_);

    // 等待直到有事件或队列已关闭
    while (true) {
        // 检查是否有事件
        int idx = get_highest_priority_queue();
        if (idx >= 0) {
            Event event = std::move(priority_queues_[idx].front());
            priority_queues_[idx].pop();
            --size_;
            return event;
        }

        // 检查队列是否已关闭
        if (queue_closed_.load(std::memory_order_acquire)) {
            break;
        }

        // 等待
        not_empty_.wait(lock);
    }

    // 队列已关闭，返回SHUTDOWN事件
    return Event::make_shutdown_event();
}

bool EventQueue::try_pop(Event& event) {
    std::lock_guard<std::mutex> lock(mutex_);

    int idx = get_highest_priority_queue();
    if (idx < 0) {
        return false;
    }

    event = std::move(priority_queues_[idx].front());
    priority_queues_[idx].pop();
    --size_;
    return true;
}

std::vector<Event> EventQueue::pop_all(size_t max_count) {
    std::vector<Event> result;
    result.reserve(max_count);

    std::lock_guard<std::mutex> lock(mutex_);

    size_t count = 0;
    while (count < max_count) {
        int idx = get_highest_priority_queue();
        if (idx < 0) {
            break;
        }

        result.push_back(std::move(priority_queues_[idx].front()));
        priority_queues_[idx].pop();
        --size_;
        ++count;
    }

    return result;
}

bool EventQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return size_ == 0;
}

size_t EventQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return size_;
}

bool EventQueue::is_closed() const {
    return queue_closed_.load(std::memory_order_acquire);
}

void EventQueue::wake_up() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_closed_.store(true, std::memory_order_release);
    }
    not_empty_.notify_all();
}

int EventQueue::get_highest_priority_queue() const {
    for (size_t i = 0; i < kEventPriorityCount; ++i) {
        if (!priority_queues_[i].empty()) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

} // namespace https_server_sim

// 文件结束
