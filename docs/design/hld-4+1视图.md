# HTTPS Server 模拟器 - 4+1 视图设计

**版本**: v1
**创建日期**: 2026-02-16
**状态**: 草稿

---

## 目录

1. [逻辑视图 (Logical View)](#1-逻辑视图-logical-view)
2. [物理视图 (Physical View)](#2-物理视图-physical-view)
3. [过程视图 (Process View)](#3-过程视图-process-view)
4. [开发视图 (Development View)](#4-开发视图-development-view)
5. [场景视图 (Scenario View)](#5-场景视图-scenario-view)

---

## 1. 逻辑视图 (Logical View)

### 1.1 系统分层架构

```
┌─────────────────────────────────────────────────────────────┐
│                     Python 3.8 适配层                        │
│  ┌──────────────────┐         ┌──────────────────┐        │
│  │   CLI (cli)      │         │   GUI (app)      │        │
│  └──────────────────┘         └──────────────────┘        │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   C++17 核心层 (core)                        │
│  ┌───────────────────────────────────────────────────────┐ │
│  │              适配层封装 (adapt)                        │ │
│  └───────────────────────────────────────────────────────┘ │
│  ┌───────────────────────────────────────────────────────┐ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌────────────┐ │ │
│  │  │  HTTP/TLS    │  │ Msg Center   │  │ Client     │ │ │
│  │  │  协议层      │  │ (Event Loop) │  │ 生命周期   │ │ │
│  │  └──────────────┘  └──────────────┘  └────────────┘ │ │
│  ├───────────────────────────────────────────────────────┤ │
│  │  ┌─────────────────────────────────────────────────┐  │ │
│  │  │          调测职责链 (Debug Chain)                │  │ │
│  │  │  延迟响应 | 强制断开 | 日志记录 | 错误码        │  │ │
│  │  └─────────────────────────────────────────────────┘  │ │
│  ├───────────────────────────────────────────────────────┤ │
│  │  ┌─────────────────────────────────────────────────┐  │ │
│  │  │       回调策略模式 (Callback Strategy)           │  │ │
│  │  │  根据server端口选择对应的回调object              │  │ │
│  │  └─────────────────────────────────────────────────┘  │ │
│  └───────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   静态链接回调 Objects                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │  Callback 1  │  │  Callback 2  │  │  Callback N  │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 核心类图

```mermaid
classDiagram
    class Server {
        +init(config_file) int
        +start() int
        +stop() int
        +get_status(status) void
        +get_statistics(stats) void
        +cleanup() void
        -init_listen_sockets() int
        -graceful_shutdown() void
        -stop_accepting() void
        -wait_pending_requests() void
        -close_all_connections() void
        -cleanup_resources() void
    }

    class ConnectionManager {
        +create_connection(fd, port) Connection*
        +get_connection(conn_id) Connection*
        +remove_connection(conn_id) void
        +get_connection_count() uint32_t
        +for_each_connection(func) void
        +check_callback_timeouts(timeout) void
    }

    class Connection {
        +get_id() uint64_t
        +get_state() ConnectionState
        +transition_to(new_state) void
        +get_fd() int
        +get_client_ip() string&
        +get_client_port() uint16_t
        +get_server_port() uint16_t
        +set_client_info(ip, port) void
        +update_last_activity() void
        +is_timeout(timeout) bool
        +set_callback_start_time() void
        +is_callback_timeout(timeout) bool
        +get_read_buffer() Buffer&
        +get_write_buffer() Buffer&
        +set_protocol_handler(handler) void
        +get_protocol_handler() ProtocolHandler*
    }

    class ProtocolHandler {
        <<interface>>
        +init(conn) int
        +on_read() int
        +on_write() int
        +send_response(data, len) int
        +close() void
        +get_tls_handler() TlsHandler*
    }

    class Http1Handler {
        +init(conn) int
        +on_read() int
        +on_write() int
        +send_response(data, len) int
        +close() void
        +get_tls_handler() TlsHandler*
        -parse_headers() int
        -parse_debug_field() int
        -handle_complete_request() int
    }

    class Http2Handler {
        +init(conn) int
        +on_read() int
        +on_write() int
        +send_response(data, len) int
        +close() void
        +get_tls_handler() TlsHandler*
        -handle_frame() int
        -handle_settings_frame(payload, len) int
        -handle_headers_frame(stream_id, payload, len, flags) int
        -handle_data_frame(stream_id, payload, len, flags) int
        -handle_window_update_frame(stream_id, increment) int
        -handle_rst_stream_frame(stream_id, error_code) int
        -get_or_create_stream(stream_id) Http2Stream*
        -handle_stream_data(stream) int
    }

    class TlsHandler {
        +init(conn) int
        +read(data, len, out_len) int
        +write(data, len, out_len) int
        +close() int
        +is_handshake_done() bool
        +continue_handshake() int
        +get_ssl() SSL*
        -init_ssl_context() int
        -configure_certificates() int
        -configure_cipher_suites() int
    }

    class MsgCenter {
        +start() int
        +stop() void
        +post_event(event) void
        +post_callback_task(task) void
        +get_event_loop() EventLoop*
    }

    class EventLoop {
        +run() void
        +stop() void
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

    class DebugChain {
        +register_handler(handler) int
        +unregister_handler(name) int
        +process_request(ctx, config, debug_ctx) int
        +process_response(ctx, config, debug_ctx) int
        -sort_handlers() void
    }

    class CallbackStrategyManager {
        +register_strategy(strategy) int
        +get_strategy(port) CallbackStrategy*
        +load_port_mapping(mapping) int
    }

    class Config {
        +load_from_file(file_path) int
        +validate() int
        +get_server() ServerConfig&
        +get_certificates() CertificatesConfig&
        +get_debug() DebugConfig&
        +get_callbacks() CallbacksConfig&
        +get_logging() LoggingConfig&
        +get_http2() Http2Config&
    }

    class Buffer {
        +write(data, len) size_t
        +read(data, len) size_t
        +peek(data, len) size_t
        +skip(len) void
        +readable_bytes() size_t
        +writable_bytes() size_t
        +ensure_writable(len) void
        +clear() void
        +read_ptr() const uint8_t*
        +write_ptr() uint8_t*
        +has_written(len) void
    }

    Server "1" *-- "1" ConnectionManager
    Server "1" *-- "1" MsgCenter
    ConnectionManager "1" *-- "*" Connection
    Connection "1" *-- "1" ProtocolHandler
    ProtocolHandler <|-- Http1Handler
    ProtocolHandler <|-- Http2Handler
    ProtocolHandler "1" *-- "1" TlsHandler
    MsgCenter "1" *-- "1" EventLoop
    MsgCenter "1" *-- "2" IoThread
    MsgCenter "1" *-- "1" WorkerPool
    Connection "1" *-- "1" Buffer
    Connection "1" *-- "1" Buffer
    Http2Handler "1" *-- "*" Http2Stream
```

---

## 2. 物理视图 (Physical View)

### 2.1 部署架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                        目标机器 (Windows/Linux/Mac)              │
│                                                                 │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │                  Python 3.8 运行环境                        │ │
│  │  ┌──────────────┐        ┌──────────────┐                │ │
│  │  │ server_sim   │        │ server_sim   │                │ │
│  │  │ _cli.py      │        │ _app.py      │                │ │
│  │  └──────────────┘        └──────────────┘                │ │
│  └───────────────────────────────────────────────────────────┘ │
│                              │                                  │
│                              ▼                                  │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │              动态库/可执行文件                              │ │
│  │  ┌─────────────────────────────────────────────────────┐  │ │
│  │  │  libhttps_server_adapt.so/.dylib/.dll               │  │ │
│  │  └─────────────────────────────────────────────────────┘  │ │
│  │  ┌─────────────────────────────────────────────────────┐  │ │
│  │  │  https_server_main (可执行文件)                      │  │ │
│  │  └─────────────────────────────────────────────────────┘  │ │
│  └───────────────────────────────────────────────────────────┘ │
│                              │                                  │
│                              ▼                                  │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │                    配置与证书文件                           │ │
│  │  ┌──────────────────┐  ┌──────────────────┐               │ │
│  │  │ server_config    │  │ cert.pem         │               │ │
│  │  │ .json            │  │ key.pem          │               │ │
│  │  └──────────────────┘  └──────────────────┘               │ │
│  └───────────────────────────────────────────────────────────┘ │
│                              │                                  │
│                              ▼                                  │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │                    日志文件                                  │ │
│  │  ┌──────────────────┐  ┌──────────────────┐               │ │
│  │  │ server.log       │  │ server.log.1     │               │ │
│  │  └──────────────────┘  └──────────────────┘               │ │
│  └───────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 组件图

```mermaid
componentDiagram
    [CLI] as cli
    [GUI] as gui
    [Adapt Layer] as adapt
    [Server Module] as server
    [Connection Module] as conn
    [Protocol Module] as protocol
    [MsgCenter Module] as msgcenter
    [DebugChain Module] as debugchain
    [Callback Module] as callback
    [Utils Module] as utils
    [Callback 1] as cb1
    [Callback 2] as cb2
    [OpenSSL] as openssl
    [JSON Library] as json

    cli --> adapt
    gui --> adapt
    adapt --> server
    adapt --> utils
    server --> conn
    server --> msgcenter
    server --> utils
    conn --> protocol
    conn --> utils
    protocol --> openssl
    protocol --> utils
    msgcenter --> utils
    debugchain --> utils
    callback --> utils
    server --> debugchain
    server --> callback
    callback --> cb1
    callback --> cb2
    utils --> json
```

---

## 3. 过程视图 (Process View)

### 3.1 线程模型图

```
┌─────────────────────────────────────────────────────────────┐
│                        进程空间                               │
│                                                              │
│  ┌───────────────────────────────────────────────────────┐ │
│  │  T1: Main/Event Loop 线程                              │ │
│  │  - Event Loop 主循环                                    │ │
│  │  - 事件分发处理                                         │ │
│  │  - 状态机管理                                           │ │
│  └───────────────────────────────────────────────────────┘ │
│                                                              │
│  ┌───────────────────────────────────────────────────────┐ │
│  │  T2: IO 线程-1                                         │ │
│  │  - epoll/select/kqueue 监听                            │ │
│  │  - 处理 accept 事件                                     │ │
│  │  - 处理 read/write 事件                                 │ │
│  └───────────────────────────────────────────────────────┘ │
│                                                              │
│  ┌───────────────────────────────────────────────────────┐ │
│  │  T3: IO 线程-2                                         │ │
│  │  - epoll/select/kqueue 监听                            │ │
│  │  - 处理 accept 事件                                     │ │
│  │  - 处理 read/write 事件                                 │ │
│  └───────────────────────────────────────────────────────┘ │
│                                                              │
│  ┌───────────────────────────────────────────────────────┐ │
│  │  T4: 工作线程-1                                        │ │
│  │  - 执行回调任务                                         │ │
│  │  - AsyncParseContent                                   │ │
│  │  - AsyncReplyContent                                   │ │
│  └───────────────────────────────────────────────────────┘ │
│                                                              │
│  ┌───────────────────────────────────────────────────────┐ │
│  │  T5: 工作线程-2                                        │ │
│  │  - 执行回调任务                                         │ │
│  │  - AsyncParseContent                                   │ │
│  │  - AsyncReplyContent                                   │ │
│  └───────────────────────────────────────────────────────┘ │
│                                                              │
│  ┌───────────────────────────────────────────────────────┐ │
│  │  共享数据结构（加锁保护）                               │ │
│  │  - EventQueue (std::mutex)                            │ │
│  │  - ConnectionManager (std::mutex)                      │ │
│  │  - CallbackStrategyManager (std::mutex)                │ │
│  └───────────────────────────────────────────────────────┘ │
│                                                              │
│  ┌───────────────────────────────────────────────────────┐ │
│  │  无锁数据结构                                           │ │
│  │  - WorkerPool TaskQueue (Lock-free)                    │ │
│  └───────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### 3.2 请求处理时序图

```mermaid
sequenceDiagram
    participant Client as Client
    participant IO as IO线程
    participant EL as EventLoop线程
    participant WP as WorkerPool
    participant WT as WorkerThread
    participant DC as DebugChain
    participant CB as Callback

    Client->>IO: TCP连接
    IO->>IO: accept()
    IO->>EL: EVT_ACCEPT事件
    activate EL
    EL->>EL: 创建Connection对象
    EL->>EL: 创建TlsHandler
    EL->>EL: TLS握手
    EL->>EL: 状态→CONNECTED
    deactivate EL

    Client->>IO: HTTP请求数据
    IO->>EL: EVT_READ事件
    activate EL
    EL->>EL: 接收请求头
    EL->>EL: 解析Debug字段
    EL->>EL: 接收请求body（分块接收，最大64MB）
    EL->>DC: DebugChain.process_request()
    activate DC
    DC->>DC: 延迟响应
    DC->>DC: 强制断开
    DC->>DC: 日志记录
    DC->>DC: 错误码
    deactivate DC
    EL->>EL: 状态→PROCESSING
    EL->>WP: 提交回调任务
    deactivate EL
    activate WP
    WP->>WT: 分配任务
    activate WT
    WT->>CB: AsyncParseContent()
    activate CB
    CB-->>WT: 返回
    deactivate CB
    WT->>CB: AsyncReplyContent()
    activate CB
    CB-->>WT: 返回响应数据
    deactivate CB
    WT->>EL: EVT_CALLBACK_DONE事件
    deactivate WT
    deactivate WP
    activate EL
    EL->>DC: DebugChain.process_response()
    activate DC
    DC->>DC: 日志记录
    deactivate DC
    EL->>EL: 状态→SENDING
    EL->>Client: 发送响应
    EL->>EL: 状态→CONNECTED
    deactivate EL
```

---

## 4. 开发视图 (Development View)

### 4.1 目录结构

```
https-server-sim/
├── docs/
│   ├── design/
│   │   ├── srs-需求分析文档.md
│   │   ├── srs-需求文档.md
│   │   ├── hld-架构设计文档.md
│   │   ├── hld-4+1视图.md
│   │   ├── hld-工程设计.md
│   │   └── modules/              # 子模块详细设计
│   │       ├── module-server.md
│   │       ├── module-connection.md
│   │       ├── module-protocol.md
│   │       ├── module-msgcenter.md
│   │       ├── module-debugchain.md
│   │       ├── module-callback.md
│   │       └── module-utils.md
│   └── .ai/
│       └── hld/                 # 架构设计检视意见
├── codes/
│   ├── api/
│   │   ├── adapt/               # C接口适配层
│   │   │   ├── include/
│   │   │   └── source/
│   │   ├── cli/                 # Python CLI适配层
│   │   │   ├── __init__.py
│   │   │   ├── main.py
│   │   │   ├── config.py
│   │   │   └── server.py
│   │   ├── app/                 # Python GUI适配层
│   │   │   ├── __init__.py
│   │   │   ├── main.py
│   │   │   ├── main_window.py
│   │   │   ├── config_panel.py
│   │   │   └── log_panel.py
│   │   └── Test/
│   │       ├── unit_tests/
│   │       └── integration_tests/
│   └── core/
│       ├── include/
│       │   ├── server/
│       │   ├── connection/
│       │   ├── protocol/
│       │   ├── msg_center/
│       │   ├── debug_chain/
│       │   ├── callback/
│       │   └── utils/
│       ├── source/
│       │   ├── server/
│       │   ├── connection/
│       │   ├── protocol/
│       │   ├── msg_center/
│       │   ├── debug_chain/
│       │   ├── callback/
│       │   └── utils/
│       └── test/
│           └── sim_client/
├── prompts/
├── scripts/
│   ├── build_project.py
│   ├── server_sim_cli.py
│   └── server_sim_app.py
├── third_party/
│   ├── openssl/
│   ├── json/
│   └── ...
└── CMakeLists.txt
```

### 4.2 包依赖图

```mermaid
graph TD
    cli[cli] --> adapt[adapt]
    gui[gui] --> adapt
    adapt --> server[server]
    adapt --> utils[utils]
    server --> connection[connection]
    server --> msgcenter[msgcenter]
    server --> utils
    connection --> protocol[protocol]
    connection --> utils
    protocol --> utils
    msgcenter --> utils
    debugchain[debugchain] --> utils
    callback[callback] --> utils
    server --> debugchain
    server --> callback
    protocol --> openssl[openssl]
    utils --> json[json]
```

---

## 5. 场景视图 (Scenario View)

### 5.1 典型用例：HTTPS请求响应

**参与者**: Client、HTTPS Server模拟器

**场景描述**: Client发起HTTPS请求，Server处理并返回响应

```mermaid
sequenceDiagram
    participant Client
    participant Server
    participant IOThread
    participant EventLoop
    participant Protocol
    participant DebugChain
    participant Callback

    Client->>Server: TCP SYN
    Server-->>Client: TCP SYN+ACK
    Client->>Server: TCP ACK
    Note over Client,Server: TCP连接建立

    Client->>Server: TLS Client Hello
    Server-->>Client: TLS Server Hello + Certificate
    Client->>Server: TLS Client Key Exchange
    Server-->>Client: TLS Finished
    Note over Client,Server: TLS握手完成

    Client->>IOThread: HTTP Request
    IOThread->>EventLoop: EVT_READ事件
    EventLoop->>Protocol: 解析请求
    Protocol->>Protocol: 解析Debug字段
    EventLoop->>DebugChain: 处理请求
    DebugChain->>DebugChain: 延迟响应
    DebugChain->>DebugChain: 日志记录
    EventLoop->>Callback: 提交回调任务
    Callback->>Callback: AsyncParseContent
    Callback->>Callback: AsyncReplyContent
    Callback->>EventLoop: 回调完成
    EventLoop->>DebugChain: 处理响应
    EventLoop->>Protocol: 发送响应
    Protocol->>Client: HTTP Response
```

### 5.2 典型用例：Graceful Shutdown

**参与者**: 用户、HTTPS Server模拟器

**场景描述**: 用户触发Server关闭，Server优雅关闭

```mermaid
sequenceDiagram
    participant User
    participant CLI
    participant Server
    participant ConnectionManager
    participant MsgCenter

    User->>CLI: server_stop()
    CLI->>Server: stop()
    activate Server
    Server->>Server: 步骤1: 停止接受新连接
    Server->>Server: 关闭监听socket
    Server->>Server: 步骤2: 处理现有请求
    Server->>ConnectionManager: 遍历连接
    Server->>Server: 等待回调完成（最长30秒）
    Server->>Server: 步骤3: 关闭连接
    Server->>ConnectionManager: 发送close_notify
    Server->>Server: 等待连接关闭（最长5秒）
    Server->>Server: 步骤4: 清理资源
    Server->>MsgCenter: 停止Event Loop
    Server->>MsgCenter: 停止IO线程
    Server->>MsgCenter: 停止工作线程
    Server-->>CLI: 返回成功
    deactivate Server
```

---

**文档结束**
