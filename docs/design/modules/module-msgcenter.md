# MsgCenter 模块详细设计

**模块名称**: MsgCenter
**模块路径**: codes/core/source/msg_center/
**版本**: v1
**创建日期**: 2026-02-16

---

## 目录

1. [模块职责](#1-模块职责)
2. [类设计](#2-类设计)
3. [接口定义](#3-接口定义)
4. [IO线程设计](#4-io线程设计)
5. [事件类型定义](#5-事件类型定义)
6. [线程模型](#6-线程模型)
7. [依赖关系](#7-依赖关系)

---

## 1. 模块职责

MsgCenter模块负责事件循环和消息分发，主要职责包括：

- Event Loop实现
- 事件优先级队列管理
- 事件分发处理
- 线程间通信
- IO线程管理
- 工作线程池管理

**该模块不负责**：
- 具体连接的处理（由Connection模块负责）
- 协议解析（由Protocol模块负责）

---

## 2. 类设计

### 2.1 Event 结构

**文件路径**: codes/core/include/msg_center/event.hpp

```cpp
namespace https_server_sim {

enum class EventType : uint8_t {
    SHUTDOWN = 0,
    ERROR = 1,
    ACCEPT = 2,
    READ = 3,
    WRITE = 4,
    CALLBACK_DONE = 5,
    TIMEOUT = 6,
    STATISTICS = 7
};

struct Event {
    EventType type;
    uint64_t conn_id;
    int fd;
    void* user_data;
    std::function<void()> handler;
};

} // namespace https_server_sim
```

### 2.2 MsgCenter 类

**文件路径**: codes/core/include/msg_center/msg_center.hpp

```cpp
namespace https_server_sim {

class MsgCenter {
public:
    MsgCenter();
    ~MsgCenter();

    // 启动消息中心
    int start();

    // 停止消息中心
    void stop();

    // 提交事件
    void post_event(const Event& event);

    // 提交回调任务
    void post_callback_task(std::function<void()> task);

    // 获取EventLoop
    EventLoop* get_event_loop() { return event_loop_.get(); }

private:
    std::unique_ptr<EventQueue> event_queue_;
    std::unique_ptr<EventLoop> event_loop_;
    std::unique_ptr<WorkerPool> worker_pool_;
    std::vector<std::unique_ptr<IoThread>> io_threads_;
    std::thread event_loop_thread_;
    std::atomic<bool> running_;
};

} // namespace https_server_sim
```

### 2.3 EventQueue 类

**文件路径**: codes/core/include/msg_center/event_queue.hpp

```cpp
namespace https_server_sim {

class EventQueue {
public:
    EventQueue(size_t max_size = 10000);
    ~EventQueue();

    // 入队
    bool push(const Event& event);

    // 出队（阻塞）
    Event pop();

    // 出队（非阻塞）
    bool try_pop(Event& event);

    // 批量出队
    std::vector<Event> pop_all(size_t max_count);

    // 队列是否为空
    bool empty() const;

    // 获取队列大小
    size_t size() const;

private:
    struct EventComparator {
        bool operator()(const Event& a, const Event& b) const {
            return static_cast<uint8_t>(a.type) > static_cast<uint8_t>(b.type);
        }
    };

    std::priority_queue<Event, std::vector<Event>, EventComparator> queue_;
    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
    size_t max_size_;
};

} // namespace https_server_sim
```

### 2.4 IoThread 类

**文件路径**: codes/core/include/msg_center/io_thread.hpp

```cpp
namespace https_server_sim {

class IoThread {
public:
    IoThread(int thread_id, EventQueue* event_queue);
    ~IoThread();

    // 启动IO线程
    void start();

    // 停止IO线程
    void stop();

    // 添加监听socket
    void add_listen_fd(int fd, uint16_t port);

    // 添加连接socket
    void add_conn_fd(int fd, uint64_t conn_id);

    // 移除socket
    void remove_fd(int fd);

private:
    // IO线程主函数
    void io_thread_func();

    // 平台特定的事件循环
    void event_loop_linux();
    void event_loop_mac();
    void event_loop_windows();

    int thread_id_;
    EventQueue* event_queue_;
    std::thread thread_;
    std::atomic<bool> running_;
    std::vector<int> listen_fds_;
    std::vector<int> conn_fds_;
};

} // namespace https_server_sim
```

### 2.5 WorkerPool 类

**文件路径**: codes/core/include/msg_center/worker_pool.hpp

```cpp
namespace https_server_sim {

class WorkerPool {
public:
    WorkerPool(size_t num_workers = 2);
    ~WorkerPool();

    // 启动工作线程
    void start();

    // 停止工作线程
    void stop();

    // 提交任务
    void post_task(std::function<void()> task);

private:
    // 工作线程主函数
    void worker_thread();

    size_t num_workers_;
    std::vector<std::thread> workers_;
    LockFreeQueue<std::function<void()>> task_queue_;
    std::atomic<bool> running_;
};

} // namespace https_server_sim
```

---

## 3. 接口定义

### 3.1 MsgCenter 类接口

| 接口 | 功能描述 | 参数 | 返回值 | 线程安全 |
|-----|---------|------|-------|---------|
| start() | 启动消息中心 | 无 | 0成功，非0错误码 | 否 |
| stop() | 停止消息中心 | 无 | 无 | 是 |
| post_event(event) | 提交事件 | event | 无 | 是 |
| post_callback_task(task) | 提交回调任务 | task | 无 | 是 |
| get_event_loop() | 获取EventLoop | 无 | EventLoop* | 是 |

### 3.2 EventQueue 类接口

| 接口 | 功能描述 | 参数 | 返回值 | 线程安全 |
|-----|---------|------|-------|---------|
| push(event) | 入队 | event | bool | 是 |
| pop() | 出队（阻塞） | 无 | Event | 是 |
| try_pop(event) | 出队（非阻塞） | event | bool | 是 |
| pop_all(max_count) | 批量出队 | max_count | vector<Event> | 是 |
| empty() | 队列是否为空 | 无 | bool | 是 |
| size() | 获取队列大小 | 无 | size_t | 是 |

### 3.3 IoThread 类接口

| 接口 | 功能描述 | 参数 | 返回值 | 线程安全 |
|-----|---------|------|-------|---------|
| start() | 启动IO线程 | 无 | 无 | 否 |
| stop() | 停止IO线程 | 无 | 无 | 是 |
| add_listen_fd(fd, port) | 添加监听socket | fd, port | 无 | 是 |
| add_conn_fd(fd, conn_id) | 添加连接socket | fd, conn_id | 无 | 是 |
| remove_fd(fd) | 移除socket | fd | 无 | 是 |

### 3.4 WorkerPool 类接口

| 接口 | 功能描述 | 参数 | 返回值 | 线程安全 |
|-----|---------|------|-------|---------|
| start() | 启动工作线程 | 无 | 无 | 否 |
| stop() | 停止工作线程 | 无 | 无 | 是 |
| post_task(task) | 提交任务 | task | 无 | 是 |

---

## 4. IO线程设计

### 4.1 IO线程职责

- 处理网络IO事件（epoll/select/kqueue）
- 处理accept事件
- 处理连接建立
- 处理read/write事件
- 向Event Loop线程提交事件

### 4.2 IO线程事件监听机制

**Linux平台（epoll）：**
- 使用epoll_create创建epoll fd
- 使用epoll_ctl注册监听socket和连接socket
- 使用epoll_wait等待事件
- 事件类型：EPOLLIN、EPOLLOUT、EPOLLERR、EPOLLHUP

**Mac平台（kqueue）：**
- 使用kqueue创建kqueue fd
- 使用kevent注册监听socket和连接socket
- 使用kevent等待事件
- 事件类型：EVFILT_READ、EVFILT_WRITE

**Windows平台（IOCP）：**
- 使用CreateIoCompletionPort创建IO完成端口
- 使用CreateFile socket
- 使用GetQueuedCompletionStatus等待事件

### 4.3 监听socket注册机制

1. 每个端口独立创建监听socket
2. 所有监听socket注册到IO线程的epoll/select/kqueue
3. IO线程-1（T2）负责奇数端口
4. IO线程-2（T3）负责偶数端口
5. accept成功后，根据监听端口获取对应的回调策略
6. 连接socket均匀分配给两个IO线程

### 4.4 事件分发逻辑

- IO线程监听到事件后，创建Event对象
- 通过队列提交给Event Loop线程
- 通过eventfd（Linux）/kqueue（Mac）/IOCP（Windows）唤醒Event Loop

---

## 5. 事件类型定义

| 事件类型 | 枚举值 | 优先级 | 说明 |
|---------|--------|-------|------|
| EVT_SHUTDOWN | 0 | 0（最高） | 系统关闭事件 |
| EVT_ERROR | 1 | 1 | 错误处理事件 |
| EVT_ACCEPT | 2 | 2 | 新连接建立事件 |
| EVT_READ | 3 | 3 | 连接可读事件 |
| EVT_WRITE | 4 | 3 | 连接可写事件 |
| EVT_CALLBACK_DONE | 5 | 4 | 回调处理完成事件 |
| EVT_TIMEOUT | 6 | 5 | 连接超时事件 |
| EVT_STATISTICS | 7 | 6（最低） | 定期统计事件 |

---

## 6. 线程模型

### 6.1 线程分工

| 线程ID | 线程名称 | 职责 |
|-------|---------|------|
| T1 | Main/Event Loop线程 | 运行Msg Center Event Loop，处理事件分发 |
| T2 | IO线程-1 | 处理网络IO事件（epoll/select/kqueue） |
| T3 | IO线程-2 | 处理网络IO事件（epoll/select/kqueue） |
| T4 | 工作线程-1 | 执行回调任务 |
| T5 | 工作线程-2 | 执行回调任务 |

### 6.2 线程间通信机制

- **通信方式**：
  - 事件队列：使用std::mutex + std::condition_variable保护
  - 任务队列：无锁队列（Lock-free Queue）
- **事件传递**：通过eventfd（Linux）/kqueue（Mac）/IOCP（Windows）唤醒Event Loop
- **任务调度**：采用FIFO调度策略

---

## 7. 依赖关系

### 7.1 依赖模块

| 模块 | 用途 |
|-----|------|
| LockFreeQueue | 无锁队列实现 |
| Event | 事件定义 |

### 7.2 依赖类图

```mermaid
classDiagram
    class MsgCenter {
        +start() int
        +stop() void
        +post_event(event) void
        +post_callback_task(task) void
        +get_event_loop() EventLoop*
    }

    class EventQueue {
        +push(event) bool
        +pop() Event
        +try_pop(event) bool
        +pop_all(max_count) vector<Event>
        +empty() bool
        +size() size_t
    }

    class IoThread {
        +start() void
        +stop() void
        +add_listen_fd(fd, port) void
        +add_conn_fd(fd, conn_id) void
        +remove_fd(fd) void
        -io_thread_func() void
        -event_loop_linux() void
        -event_loop_mac() void
        -event_loop_windows() void
    }

    class WorkerPool {
        +start() void
        +stop() void
        +post_task(task) void
        -worker_thread() void
    }

    class Event {
        +EventType type
        +uint64_t conn_id
        +int fd
        +void* user_data
        +function<void()> handler
    }

    class LockFreeQueue~T~ {
        +push(item) void
        +pop(item) bool
        +empty() bool
    }

    MsgCenter "1" *-- "1" EventQueue
    MsgCenter "1" *-- "2" IoThread
    MsgCenter "1" *-- "1" WorkerPool
    WorkerPool "1" *-- "1" LockFreeQueue
    EventQueue "1" *-- "*" Event
```

---

**文档结束**
