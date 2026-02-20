// =============================================================================
//  HTTPS Server Simulator - MsgCenter Module
//  文件: worker_pool.cpp
//  描述: WorkerPool工作线程池类实现
//  版权: Copyright (c) 2026
// =============================================================================
#include "msg_center/worker_pool.hpp"
#include "utils/logger.hpp"
#include <chrono>

namespace https_server_sim {

WorkerPool::WorkerPool(size_t num_workers, EventLoop* event_loop)
    : num_workers_(num_workers)
    , running_(false)
    , queue_closed_(false)
    , event_loop_(event_loop)
    , post_callback_done_(false)
{}

WorkerPool::~WorkerPool() {
    stop();
}

void WorkerPool::start() {
    if (running_.load(std::memory_order_acquire)) {
        return;
    }

    running_.store(true, std::memory_order_release);
    queue_closed_.store(false, std::memory_order_release);

    workers_.reserve(num_workers_);
    for (size_t i = 0; i < num_workers_; ++i) {
        workers_.emplace_back(&WorkerPool::worker_thread, this);
    }
}

void WorkerPool::stop() {
    if (!running_.load(std::memory_order_acquire)) {
        return;
    }

    running_.store(false, std::memory_order_release);
    queue_closed_.store(true, std::memory_order_release);

    // 唤醒所有工作线程（通知在锁外发送，避免惊群）
    task_cv_.notify_all();

    // 等待所有工作线程退出
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
}

void WorkerPool::post_task(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        task_queue_.push(std::move(task));
    }
    task_cv_.notify_one();
}

size_t WorkerPool::get_thread_count() const {
    return num_workers_;
}

void WorkerPool::set_post_callback_done(bool enable) {
    post_callback_done_.store(enable, std::memory_order_release);
}

void WorkerPool::worker_thread() {
    while (true) {
        std::function<void()> task;
        bool has_task = false;

        {
            std::unique_lock<std::mutex> lock(mutex_);

            // 等待任务或停止信号
            // 注意：wait谓词在锁保护下执行，可以安全读取task_queue_
            task_cv_.wait(lock, [this]() {
                // 在锁保护下检查队列，running_和queue_closed_用原子读取
                // 只要队列非空，或者队列关闭且不运行了，就返回
                return !task_queue_.empty() ||
                       queue_closed_.load(std::memory_order_acquire) ||
                       !running_.load(std::memory_order_acquire);
            });

            // 先尝试获取任务（即使已经停止，也要处理完队列中剩余的任务）
            if (!task_queue_.empty()) {
                task = std::move(task_queue_.front());
                task_queue_.pop();
                has_task = true;
            }

            // 只有当队列已空且不再运行时，才退出循环
            if (!has_task && !running_.load(std::memory_order_acquire)) {
                break;
            }
        }

        // 执行任务（在锁外执行，避免阻塞其他线程）
        if (has_task) {
            try {
                task();
            } catch (const std::exception& e) {
                // 捕获标准异常，记录详细信息
                LOG_ERROR("WorkerPool", "Task threw exception: %s", e.what());
            } catch (...) {
                // 捕获所有其他异常
                LOG_ERROR("WorkerPool", "Task threw unknown exception");
            }

            // 任务执行完成后，可选投递CALLBACK_DONE事件
            if (post_callback_done_.load(std::memory_order_acquire) &&
                event_loop_ != nullptr) {
                post_callback_done_event();
            }
        }
    }
}

void WorkerPool::post_callback_done_event() {
    if (event_loop_ == nullptr) {
        return;
    }
    event_loop_->post_event(Event::make_callback_done_event());
}

} // namespace https_server_sim

// 文件结束
