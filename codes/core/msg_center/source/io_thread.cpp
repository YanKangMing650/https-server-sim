// =============================================================================
//  HTTPS Server Simulator - MsgCenter Module
//  文件: io_thread.cpp
//  描述: IoThread IO线程类实现
//  版权: Copyright (c) 2026
// =============================================================================
#include "msg_center/io_thread.hpp"
#include <chrono>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

namespace https_server_sim {

// ============================================================================
//  内部工具函数 (namespace details)
// ============================================================================
namespace details {

/**
 * @brief 设置文件描述符为非阻塞模式
 * @param fd 文件描述符
 * @return true-成功，false-失败
 */
inline bool SetNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return false;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return false;
    }
    return true;
}

} // namespace details

// ============================================================================
//  IoThread构造函数与析构函数
// ============================================================================

IoThread::IoThread(int thread_id, EventQueue* event_queue)
    : thread_id_(thread_id)
    , event_queue_(event_queue)
    , running_(false)
    , epoll_fd_(-1)
    , kq_fd_(-1)
    , iocp_handle_(nullptr)
    , wakeup_fd_(-1)
    , wakeup_read_fd_(-1)
{
}

IoThread::~IoThread() {
    stop();
}

// ============================================================================
//  IoThread启动与停止
// ============================================================================

void IoThread::start() {
    if (running_.load(std::memory_order_acquire)) {
        return;
    }

    running_.store(true, std::memory_order_release);
    thread_ = std::thread(&IoThread::io_thread_func, this);
}

void IoThread::stop() {
    if (!running_.load(std::memory_order_acquire)) {
        return;
    }

    running_.store(false, std::memory_order_release);
    wake_up();

    if (thread_.joinable()) {
        thread_.join();
    }
}

// ============================================================================
//  IoThread fd管理
// ============================================================================

void IoThread::add_listen_fd(int fd, uint16_t port) {
    std::lock_guard<std::mutex> lock(fd_mutex_);

    listen_fd_to_port_[fd] = port;
    // 同步添加到listen_fds_（避免重复）
    auto it = std::find(listen_fds_.begin(), listen_fds_.end(), fd);
    if (it == listen_fds_.end()) {
        listen_fds_.push_back(fd);
    }

    wake_up();
}

void IoThread::add_conn_fd(int fd, uint64_t conn_id) {
    std::lock_guard<std::mutex> lock(fd_mutex_);

    fd_to_conn_id_[fd] = conn_id;
    // 同步添加到conn_fds_（避免重复）
    auto it = std::find(conn_fds_.begin(), conn_fds_.end(), fd);
    if (it == conn_fds_.end()) {
        conn_fds_.push_back(fd);
    }

    wake_up();
}

void IoThread::remove_fd(int fd) {
    std::lock_guard<std::mutex> lock(fd_mutex_);

    // 从listen数据结构中移除
    auto listen_it = listen_fd_to_port_.find(fd);
    if (listen_it != listen_fd_to_port_.end()) {
        listen_fd_to_port_.erase(listen_it);
        // 同步从listen_fds_中移除
        auto vec_it = std::find(listen_fds_.begin(), listen_fds_.end(), fd);
        if (vec_it != listen_fds_.end()) {
            listen_fds_.erase(vec_it);
        }
    }

    // 从conn数据结构中移除
    auto conn_it = fd_to_conn_id_.find(fd);
    if (conn_it != fd_to_conn_id_.end()) {
        fd_to_conn_id_.erase(conn_it);
        // 同步从conn_fds_中移除
        auto vec_it = std::find(conn_fds_.begin(), conn_fds_.end(), fd);
        if (vec_it != conn_fds_.end()) {
            conn_fds_.erase(vec_it);
        }
    }

    wake_up();
}

// ============================================================================
//  IoThread主函数
// ============================================================================

void IoThread::io_thread_func() {
    // 根据平台选择事件循环
#ifdef __APPLE__
    event_loop_mac();
#elif defined(__linux__)
    event_loop_linux();
#elif defined(_WIN32)
    event_loop_windows();
#else
    // 默认实现：简单轮询
    while (running_.load(std::memory_order_acquire)) {
        if (event_queue_ == nullptr) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
#endif
}

// ============================================================================
//  IoThread macOS kqueue实现
// ============================================================================

void IoThread::event_loop_mac() {
    // 1. 创建kqueue
    kq_fd_ = kqueue();
    if (kq_fd_ == -1) {
        // kqueue创建失败，降级到简单轮询
        while (running_.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return;
    }

    // 2. 创建pipe用于唤醒
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1) {
        close(kq_fd_);
        kq_fd_ = -1;
        while (running_.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return;
    }

    wakeup_read_fd_ = pipe_fds[0];
    wakeup_fd_ = pipe_fds[1];

    // 设置pipe为非阻塞
    details::SetNonBlocking(wakeup_read_fd_);
    details::SetNonBlocking(wakeup_fd_);

    // 3. 注册pipe读端到kqueue
    struct kevent changelist[1];
    EV_SET(&changelist[0], wakeup_read_fd_, EVFILT_READ, EV_ADD, 0, 0, nullptr);

    if (kevent(kq_fd_, changelist, 1, nullptr, 0, nullptr) == -1) {
        close(wakeup_read_fd_);
        close(wakeup_fd_);
        close(kq_fd_);
        wakeup_read_fd_ = -1;
        wakeup_fd_ = -1;
        kq_fd_ = -1;
        while (running_.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return;
    }

    // 事件循环主逻辑
    const int kMaxEvents = 64;
    struct kevent eventlist[kMaxEvents];

    while (running_.load(std::memory_order_acquire)) {
        // 每次循环开始时，重新注册所有fd到kqueue
        // 这种设计简化了线程安全问题
        {
            std::lock_guard<std::mutex> lock(fd_mutex_);

            // 清空并重新注册所有fd
            // 使用EV_CLEAR标志，这样每次事件触发后需要重新注册
            // 这样可以避免事件不断触发

            // 注册listen fd（仅监听读事件）
            for (int fd : listen_fds_) {
                struct kevent kev;
                EV_SET(&kev, fd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, nullptr);
                kevent(kq_fd_, &kev, 1, nullptr, 0, nullptr);
            }

            // 注册conn fd（监听读和写事件）
            for (int fd : conn_fds_) {
                struct kevent kev_read;
                EV_SET(&kev_read, fd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, nullptr);
                kevent(kq_fd_, &kev_read, 1, nullptr, 0, nullptr);

                struct kevent kev_write;
                EV_SET(&kev_write, fd, EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, nullptr);
                kevent(kq_fd_, &kev_write, 1, nullptr, 0, nullptr);
            }
        }

        // 等待事件（超时100ms，便于检查running_标志）
        struct timespec timeout;
        timeout.tv_sec = 0;
        timeout.tv_nsec = 100 * 1000 * 1000; // 100ms

        int n = kevent(kq_fd_, nullptr, 0, eventlist, kMaxEvents, &timeout);

        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            // 出错，短暂休眠后继续
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (n == 0) {
            // 超时，继续循环
            continue;
        }

        // 处理触发的事件
        for (int i = 0; i < n; ++i) {
            int fd = static_cast<int>(eventlist[i].ident);
            int filter = eventlist[i].filter;
            uint32_t flags = eventlist[i].flags;

            // 检查是否是唤醒pipe
            if (fd == wakeup_read_fd_) {
                // 读取pipe中的数据，清空唤醒事件
                char buf[32];
                while (read(wakeup_read_fd_, buf, sizeof(buf)) > 0) {
                    // 持续读取直到清空
                }
                continue;
            }

            // 检查错误事件
            if (flags & EV_ERROR) {
                continue;
            }

            // 处理EOF事件（使用ERROR事件替代未定义的CLOSE）
            if (flags & EV_EOF) {
                std::lock_guard<std::mutex> lock(fd_mutex_);
                auto it = fd_to_conn_id_.find(fd);
                if (it != fd_to_conn_id_.end() && event_queue_ != nullptr) {
                    Event event;
                    event.type = EventType::ERROR;
                    event.fd = fd;
                    event.conn_id = it->second;
                    event_queue_->push(event);
                }
                continue;
            }

            // 处理读事件
            if (filter == EVFILT_READ) {
                std::lock_guard<std::mutex> lock(fd_mutex_);

                // 检查是否是listen fd
                auto listen_it = listen_fd_to_port_.find(fd);
                if (listen_it != listen_fd_to_port_.end()) {
                    // 关联用例：IO-ACCEPT-001（功能用例）：监听socket接受新连接
                    // listen fd可读，调用accept
                    if (event_queue_ != nullptr) {
                        Event event;
                        event.type = EventType::ACCEPT;
                        event.fd = fd;
                        event.conn_id = 0;
                        event_queue_->push(event);
                    }
                } else {
                    // 检查是否是conn fd
                    auto conn_it = fd_to_conn_id_.find(fd);
                    if (conn_it != fd_to_conn_id_.end() && event_queue_ != nullptr) {
                        // 关联用例：IO-READ-001（功能用例）：连接socket可读
                        Event event;
                        event.type = EventType::READ;
                        event.fd = fd;
                        event.conn_id = conn_it->second;
                        event_queue_->push(event);
                    }
                }
            }

            // 处理写事件
            if (filter == EVFILT_WRITE) {
                std::lock_guard<std::mutex> lock(fd_mutex_);

                // 检查是否是conn fd
                auto conn_it = fd_to_conn_id_.find(fd);
                if (conn_it != fd_to_conn_id_.end() && event_queue_ != nullptr) {
                    // 关联用例：IO-WRITE-001（功能用例）：连接socket可写
                    Event event;
                    event.type = EventType::WRITE;
                    event.fd = fd;
                    event.conn_id = conn_it->second;
                    event_queue_->push(event);
                }
            }
        }
    }

    // 清理资源
    if (wakeup_read_fd_ != -1) {
        close(wakeup_read_fd_);
        wakeup_read_fd_ = -1;
    }
    if (wakeup_fd_ != -1) {
        close(wakeup_fd_);
        wakeup_fd_ = -1;
    }
    if (kq_fd_ != -1) {
        close(kq_fd_);
        kq_fd_ = -1;
    }
}

// ============================================================================
//  IoThread Linux epoll实现（保留框架）
// ============================================================================

void IoThread::event_loop_linux() {
    // Linux epoll实现框架（桩代码，预留完整实现位置）
    while (running_.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// ============================================================================
//  IoThread Windows IOCP实现（保留框架）
// ============================================================================

void IoThread::event_loop_windows() {
    // Windows IOCP实现框架（桩代码，预留完整实现位置）
    while (running_.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// ============================================================================
//  IoThread唤醒机制
// ============================================================================

void IoThread::wake_up() {
    // 平台特定唤醒机制
#ifdef __APPLE__
    if (wakeup_fd_ != -1) {
        // 写入一个字节到pipe来唤醒kqueue
        char buf = 'W';
        ssize_t written = write(wakeup_fd_, &buf, 1);
        (void)written; // 忽略返回值
    }
#elif defined(__linux__)
    // Linux: write to eventfd (预留)
    (void)0;
#elif defined(_WIN32)
    // Windows: PostQueuedCompletionStatus (预留)
    (void)0;
#else
    (void)0;
#endif
}

} // namespace https_server_sim

// 文件结束
