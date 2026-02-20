// =============================================================================
//  HTTPS Server Simulator - MsgCenter Module
//  文件: msg_center.cpp
//  描述: MsgCenter模块门面类实现
//  版权: Copyright (c) 2026
// =============================================================================
#include "msg_center/msg_center.hpp"
#include "utils/logger.hpp"
#include <algorithm>

namespace https_server_sim {

// MsgCenterError转字符串函数实现
const char* msg_center_error_to_string(MsgCenterError error) {
    switch (error) {
        case MsgCenterError::SUCCESS:
            return "SUCCESS";
        case MsgCenterError::ALREADY_RUNNING:
            return "ALREADY_RUNNING";
        case MsgCenterError::THREAD_CREATE_FAILED:
            return "THREAD_CREATE_FAILED";
        case MsgCenterError::INVALID_PARAMETER:
            return "INVALID_PARAMETER";
        case MsgCenterError::NOT_FOUND:
            return "NOT_FOUND";
        default:
            return "UNKNOWN_ERROR";
    }
}

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
        event_queue_ = std::make_shared<EventQueue>();

        // 创建EventLoop（传入EventQueue指针）
        event_loop_ = std::make_shared<EventLoop>(event_queue_.get());

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

    // 加锁保护，与post_event()互斥
    std::lock_guard<std::mutex> lock(post_mutex_);

    running_.store(false, std::memory_order_release);

    // 先停止WorkerPool，确保WorkerPool的回调完成事件能被EventLoop处理
    if (worker_pool_) {
        worker_pool_->stop();
    }

    // 停止EventLoop
    if (event_loop_) {
        event_loop_->stop();
    }

    // 等待EventLoop线程结束
    if (event_loop_thread_.joinable()) {
        event_loop_thread_.join();
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
    // 加锁保护，防止与stop()中的reset操作产生竞态
    std::lock_guard<std::mutex> lock(post_mutex_);

    // 检查运行状态，非运行状态下不投递事件
    if (!running_.load(std::memory_order_acquire)) {
        return;
    }

    // 投递路径：优先通过event_loop_投递，否则直接投递到event_queue_
    if (event_loop_) {
        event_loop_->post_event(event);
    } else if (event_queue_) {
        event_queue_->push(event);
    }
}

void MsgCenter::post_callback_task(std::function<void()> task) {
    if (worker_pool_) {
        worker_pool_->post_task(std::move(task));
    } else {
        // worker_pool_ 为 nullptr 时记录错误日志
        LOG_ERROR("MsgCenter", "post_callback_task called but worker_pool_ is null, task dropped");
    }
}

int MsgCenter::add_listen_fd(int fd) {
    return add_listen_fd(fd, 0);
}

int MsgCenter::add_listen_fd(int fd, uint16_t port) {
    if (fd < 0) {
        return static_cast<int>(MsgCenterError::INVALID_PARAMETER);
    }

    // 维护内部列表（加锁保护）
    {
        std::lock_guard<std::mutex> lock(listen_fds_mutex_);
        auto it = std::find(listen_fds_.begin(), listen_fds_.end(), fd);
        if (it == listen_fds_.end()) {
            listen_fds_.push_back(fd);
        }
    }

    // 将fd添加到所有IoThread中（MC-002修复）
    for (auto& io_thread : io_threads_) {
        if (io_thread) {
            io_thread->add_listen_fd(fd, port);
        }
    }

    return static_cast<int>(MsgCenterError::SUCCESS);
}

int MsgCenter::remove_listen_fd(int fd) {
    bool existed = false;
    // 从内部列表移除（加锁保护）
    {
        std::lock_guard<std::mutex> lock(listen_fds_mutex_);
        auto it = std::find(listen_fds_.begin(), listen_fds_.end(), fd);
        if (it != listen_fds_.end()) {
            listen_fds_.erase(it);
            existed = true;
        }
    }

    // 从所有IoThread中移除fd（MC-002修复）
    for (auto& io_thread : io_threads_) {
        if (io_thread) {
            io_thread->remove_fd(fd);
        }
    }

    // 根据fd是否在内部列表中返回不同结果
    if (existed) {
        return static_cast<int>(MsgCenterError::SUCCESS);
    } else {
        return static_cast<int>(MsgCenterError::NOT_FOUND);
    }
}

void MsgCenter::get_statistics(utils::Statistics* stats) const {
    if (stats == nullptr) {
        return;
    }
    // 使用StatisticsManager获取统计信息
    utils::StatisticsManager::instance().get_statistics(stats);
}

} // namespace https_server_sim

// 文件结束
