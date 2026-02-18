// =============================================================================
//  HTTPS Server Simulator - MsgCenter Module
//  文件: msg_center.cpp
//  描述: MsgCenter模块门面类实现
//  版权: Copyright (c) 2026
// =============================================================================
#include "msg_center/msg_center.hpp"

namespace https_server_sim {

MsgCenter::MsgCenter(size_t io_thread_count, size_t worker_thread_count)
    : running_(false)
    , io_thread_count_(io_thread_count)
    , worker_thread_count_(worker_thread_count)
{}

MsgCenter::~MsgCenter() {
    stop();
}

int MsgCenter::start() {
    // 检查是否已在运行
    if (running_.load(std::memory_order_acquire)) {
        return static_cast<int>(MsgCenterError::ALREADY_RUNNING);
    }

    // 检查参数有效性
    if (io_thread_count_ == 0 || worker_thread_count_ == 0) {
        return static_cast<int>(MsgCenterError::INVALID_PARAMETER);
    }

    try {
        // 创建EventQueue
        event_queue_ = std::make_unique<EventQueue>();

        // 创建EventLoop（传入EventQueue指针）
        event_loop_ = std::make_unique<EventLoop>(event_queue_.get());

        // 创建WorkerPool（传入worker_thread_count_和EventLoop指针）
        // WorkerPool构造时post_callback_done_默认为false
        worker_pool_ = std::make_unique<WorkerPool>(worker_thread_count_, event_loop_.get());

        // 创建io_thread_count_个IoThread（每个传入EventQueue指针）
        io_threads_.reserve(io_thread_count_);
        for (size_t i = 0; i < io_thread_count_; ++i) {
            io_threads_.push_back(std::make_unique<IoThread>(static_cast<int>(i), event_queue_.get()));
        }

        // 启动WorkerPool
        worker_pool_->start();

        // 启动所有IoThread
        for (auto& io_thread : io_threads_) {
            io_thread->start();
        }

        // 在独立线程启动EventLoop::run()
        event_loop_thread_ = std::thread([this]() {
            if (event_loop_) {
                event_loop_->run();
            }
        });

        // 可靠等待EventLoop线程启动
        if (!event_loop_->wait_for_started(1000)) {
            // 超时，清理资源
            stop();
            return static_cast<int>(MsgCenterError::THREAD_CREATE_FAILED);
        }

        // 调用worker_pool_->set_post_callback_done(true)
        // 此时EventLoop已启动，安全启用回调完成事件投递
        worker_pool_->set_post_callback_done(true);

        // 设置running_ = true
        running_.store(true, std::memory_order_release);

        return static_cast<int>(MsgCenterError::SUCCESS);
    } catch (const std::exception&) {
        // 清理已创建的资源
        stop();
        return static_cast<int>(MsgCenterError::THREAD_CREATE_FAILED);
    }
}

void MsgCenter::stop() {
    if (!running_.load(std::memory_order_acquire)) {
        return;
    }

    running_.store(false, std::memory_order_release);

    // 停止EventLoop
    if (event_loop_) {
        event_loop_->stop();
    }

    // 等待EventLoop线程结束
    if (event_loop_thread_.joinable()) {
        event_loop_thread_.join();
    }

    // 停止WorkerPool
    if (worker_pool_) {
        worker_pool_->stop();
    }

    // 停止所有IoThread
    for (auto& io_thread : io_threads_) {
        if (io_thread) {
            io_thread->stop();
        }
    }
    io_threads_.clear();

    // 清理资源
    worker_pool_.reset();
    event_loop_.reset();
    event_queue_.reset();
}

void MsgCenter::post_event(const Event& event) {
    // 统一投递路径：优先通过event_loop_投递，否则直接投递到event_queue_
    if (event_loop_) {
        event_loop_->post_event(event);
    } else if (event_queue_) {
        event_queue_->push(event);
    }
}

void MsgCenter::post_callback_task(std::function<void()> task) {
    if (worker_pool_) {
        worker_pool_->post_task(std::move(task));
    }
}

} // namespace https_server_sim

// 文件结束
