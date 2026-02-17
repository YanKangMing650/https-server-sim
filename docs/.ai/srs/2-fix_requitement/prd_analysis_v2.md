# HTTPS Server 模拟器 - 产品规格分析文档

**版本**: v2
**创建日期**: 2026-02-15
**状态**: 迭代版本 - 修复v1检视问题

---

## 目录

1. [产品定位分析](#1-产品定位分析)
2. [目标用户分析](#2-目标用户分析)
3. [系统架构规格](#3-系统架构规格)
4. [功能规格详细设计](#4-功能规格详细设计)
5. [非功能规格详细设计](#5-非功能规格详细设计)
6. [接口规格设计](#6-接口规格设计)
7. [配置规格设计](#7-配置规格设计)
8. [项目目录结构规格](#8-项目目录结构规格)
9. [验收标准定义](#9-验收标准定义)

---

## 1. 产品定位分析

### 1.1 产品核心价值

HTTPS Server 模拟器是一个面向开发者的测试工具，核心价值在于：
- 提供可配置的HTTPS服务端模拟环境
- 支持用户通过C接口自定义报文处理逻辑
- 支持高并发场景下的Client端测试
- 提供调测能力辅助问题定位

### 1.2 产品边界

**包含范围**：
- C++17实现的HTTPS Server核心
- Python 3.8 CLI/GUI适配层
- C接口回调机制
- 测试用Client模拟器
- 多证书支持（国际证书、国密证书、自定义证书）

**不包含范围**：
- 版本升级机制
- 高可靠性保障
- 安全加固

---

## 2. 目标用户分析

### 2.1 用户画像

| 用户类型 | 使用场景 | 核心需求 |
|---------|---------|---------|
| 协议开发工程师 | 测试自行开发的HTTPS Client | 灵活的报文自定义能力 |
| 性能测试工程师 | 验证Client高并发表现 | 支持1000并发连接 |
| 集成测试工程师 | 端到端功能验证 | 可配置的调测能力 |

---

## 3. 系统架构规格

### 3.1 分层架构规格

系统采用三层架构设计，各层规格定义如下：

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

### 3.2 各层职责规格

| 层级 | 模块 | 职责定义 |
|-----|------|---------|
| Python适配层 | CLI | 命令行界面，解析命令行参数，加载配置，启动/停止Server |
| Python适配层 | GUI | 图形界面，提供配置编辑、Server控制、状态展示功能 |
| C++核心层 | adapt | 适配层封装，提供C API供Python调用 |
| C++核心层 | HTTP/TLS协议层 | 处理TCP连接、TLS握手、HTTP协议解析 |
| C++核心层 | Msg Center | Event Loop机制，消息分发，任务调度 |
| C++核心层 | Client生命周期 | 连接状态管理，超时处理 |
| C++核心层 | 调测职责链 | 按顺序执行各调测点，可配置开启/关闭 |
| C++核心层 | 回调策略 | 根据端口选择对应的回调object |
| 回调层 | Callback Objects | 用户实现的回调函数集合 |

### 3.3 线程模型规格

#### 3.3.1 线程分工规格

系统采用固定5线程模型，各线程分工如下：

| 线程ID | 线程名称 | 职责 |
|-------|---------|------|
| T1 | Main/Event Loop线程 | 运行Msg Center Event Loop，处理事件分发 |
| T2 | IO线程-1 | 处理网络IO事件（epoll/select/kqueue） |
| T3 | IO线程-2 | 处理网络IO事件（epoll/select/kqueue） |
| T4 | 工作线程-1 | 执行回调任务 |
| T5 | 工作线程-2 | 执行回调任务 |

#### 3.3.2 线程间通信机制

- **通信方式**: 无锁队列（Lock-free Queue）
- **事件传递**: 通过eventfd（Linux）/kqueue（Mac）/IOCP（Windows）唤醒Event Loop
- **任务调度**: 采用FIFO调度策略

---

## 4. 功能规格详细设计

### 4.1 HTTPS Server核心功能规格

| 功能ID | 功能名称 | 详细规格 |
|-------|---------|---------|
| FS-4.1.1 | TLS连接管理 | 支持TLS 1.2、TLS 1.3，可配置；<br>TLS 1.2支持的Cipher Suites：<br>  - TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256<br>  - TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384<br>  - TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256<br>  - TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384<br>TLS 1.3支持的Cipher Suites：<br>  - TLS_AES_128_GCM_SHA256<br>  - TLS_AES_256_GCM_SHA384<br>  - TLS_CHACHA20_POLY1305_SHA256 |
| FS-4.1.2 | 证书管理 | 支持国际证书、国密SM2证书、自定义证书；<br>证书格式：PEM格式；<br>国密SM2证书遵循GM/T 0009-2012标准；<br>自定义证书指用户提供的任意X.509证书 |
| FS-4.1.3 | HTTP协议支持 | 支持HTTP/1.1、HTTP/2及所有版本 |
| FS-4.1.4 | HTTP方法支持 | 支持GET、POST、PUT、DELETE等所有HTTP方法 |
| FS-4.1.5 | 报文透传 | HTTPS body作为二进制流透传，不做解析；<br>单报文最大支持长度：64MB |
| FS-4.1.6 | 响应返回 | 从回调获取响应数据，原样返回给client |

### 4.2 并发与连接管理规格

| 功能ID | 功能名称 | 详细规格 |
|-------|---------|---------|
| FS-4.2.1 | 进程模型 | 单进程设计，最多1个进程 |
| FS-4.2.2 | 线程模型 | 进程内至多5个线程，线程分工见3.3节 |
| FS-4.2.3 | 连接容量 | 同时支持1000个client连接；<br>1000连接下内存消耗上限：512MB；<br>1000连接下CPU使用率上限（空闲时）：5% |
| FS-4.2.4 | 连接建立判定 | TCP三次握手 + TLS认证成功 = 连接建立 |
| FS-4.2.5 | 连接超时 | 默认30秒无活动断开，可配置；<br>配置范围：1-3600秒 |
| FS-4.2.6 | 连接断开处理 | 超时后直接关闭TCP连接 |
| FS-4.2.7 | 生命周期跟踪 | 跟踪client所有状态，状态机定义见4.2.8节 |

#### 4.2.8 连接生命周期状态机规格

**状态定义**：

| 状态 | 状态名称 | 说明 |
|-----|---------|------|
| S0 | INIT | 初始状态，socket已创建但未开始accept |
| S1 | ACCEPTING | TCP连接建立中，等待三次握手完成 |
| S2 | TLS_HANDSHAKING | TLS握手进行中 |
| S3 | CONNECTED | 连接已建立，等待接收数据 |
| S4 | RECEIVING | 正在接收请求报文 |
| S5 | PROCESSING | 报文处理中（回调处理中） |
| S6 | SENDING | 正在发送响应报文 |
| S7 | DISCONNECTING | 连接断开中 |
| S8 | DISCONNECTED | 连接已断开 |

**状态转换规则**：

```
S0 → S1: accept()调用成功
S1 → S2: TCP三次握手完成
S2 → S3: TLS握手成功
S3 → S4: 接收到HTTP请求头
S4 → S5: 请求报文接收完成
S5 → S6: 回调返回响应数据
S6 → S3: 响应发送完成，等待下一个请求
S3 → S7: 超时触发
S4 → S7: 超时触发
S5 → S7: 超时触发
S6 → S7: 超时触发
S1 → S7: TCP连接失败
S2 → S7: TLS握手失败
S7 → S8: TCP连接关闭完成
S8 → S0: 连接资源释放完成
```

### 4.3 C接口回调功能规格

| 功能ID | 功能名称 | 详细规格 |
|-------|---------|---------|
| FS-4.3.1 | 静态链接方式 | 回调通过CMake object静态链接 |
| FS-4.3.2 | 策略模式 | 支持多个回调object，通过server端口选择策略 |
| FS-4.3.3 | 异步处理 | 回调采用异步方式处理报文 |
| FS-4.3.4 | Event Loop | msg_center采用event loop机制 |

### 4.4 Debug调测功能规格

| 功能ID | 功能名称 | 详细规格 |
|-------|---------|---------|
| FS-4.4.1 | Debug字段解析 | 解析HTTP header末尾的`<debug-server-sim>`字段；<br>字段位置：HTTP header的最后一个header；<br>编码格式：UTF-8；<br>解析规则：见4.4.7节 |
| FS-4.4.2 | 延迟响应 | 支持配置延迟响应时间，单位毫秒；<br>取值范围：0-60000ms；<br>精度：±10ms |
| FS-4.4.3 | 强制断开 | 支持强制断开client连接 |
| FS-4.4.4 | 报文日志 | 支持记录报文日志 |
| FS-4.4.5 | 错误码返回 | 支持返回特定HTTP错误码 |
| FS-4.4.6 | 职责链模式 | 调测点采用职责链模式，可叠加，支持扩展；<br>执行顺序、配置方式见4.4.7节 |

#### 4.4.7 调测职责链规格

**职责链执行顺序**：

```
请求 → [延迟响应] → [强制断开] → [日志记录] → [错误码] → 回调处理
```

**各调测点配置规则**：

| 调测点 | 配置字段 | 类型 | 取值范围 | 默认值 | 说明 |
|-------|---------|------|---------|--------|------|
| 延迟响应 | delay_ms | integer | 0-60000 | 0 | 延迟毫秒数，0表示不延迟 |
| 强制断开 | force_disconnect | boolean | true/false | false | 处理完后是否强制断开连接 |
| 日志记录 | log_packet | boolean | true/false | false | 是否记录请求/响应报文 |
| 错误码返回 | http_status | integer | 100-599 | 200 | 返回的HTTP状态码 |

**调测点数据传递规则**：
- 各调测点按顺序执行
- 前一个调测点的输出作为后一个调测点的输入
- 若某个调测点抛出异常，中断职责链执行，返回错误响应
- 各调测点可通过配置独立开启/关闭，关闭的调测点直接跳过

**Debug字段解析规则**：

Debug字段格式：
```
<debug-server-sim>
{
    "delay_ms": 100,
    "force_disconnect": false,
    "log_packet": true,
    "http_status": 200
}
</debug-server-sim>
```

解析规则：
1. 查找HTTP header中以`<debug-server-sim>`开头、以`</debug-server-sim>`结尾的header值
2. 提取中间的JSON字符串
3. 解析JSON，获取各调测点配置
4. 若解析失败，忽略该Debug字段，使用默认配置
5. 若存在多个Debug字段，使用第一个有效字段

### 4.5 适配层功能规格

| 功能ID | 功能名称 | 详细规格 |
|-------|---------|---------|
| FS-4.5.1 | CLI界面 | Python 3.8命令行界面 |
| FS-4.5.2 | GUI界面 | Python 3.8图形界面（Windows使用） |
| FS-4.5.3 | 配置文件 | JSON格式配置文件 |

### 4.6 测试用Client模拟器规格

| 功能ID | 功能名称 | 详细规格 |
|-------|---------|---------|
| FS-4.6.1 | 实现语言 | 全Python实现，不对外发布 |
| FS-4.6.2 | HTTPS支持 | 支持HTTPS，可配置证书 |
| FS-4.6.3 | HTTP版本 | 支持HTTP/1.1和HTTP/2 |
| FS-4.6.4 | 报文配置 | 通过JSON Schema配置文件生成报文，详细规格见4.6.5节 |
| FS-4.6.5 | 连接管理 | 支持建联、发送报文、接收响应 |

#### 4.6.5 测试用Client JSON Schema配置规格

```json
{
    "$schema": "http://json-schema.org/draft-07/schema#",
    "type": "object",
    "properties": {
        "server": {
            "type": "object",
            "properties": {
                "host": {
                    "type": "string",
                    "description": "Server主机名或IP地址",
                    "default": "127.0.0.1"
                },
                "port": {
                    "type": "integer",
                    "minimum": 1,
                    "maximum": 65535,
                    "description": "Server端口号",
                    "default": 8443
                },
                "tls_version": {
                    "type": "string",
                    "enum": ["1.2", "1.3"],
                    "description": "TLS版本",
                    "default": "1.2"
                },
                "ca_cert": {
                    "type": "string",
                    "description": "CA证书文件路径，可选",
                    "default": ""
                }
            },
            "required": ["host", "port", "tls_version"]
        },
        "client": {
            "type": "object",
            "properties": {
                "http_version": {
                    "type": "string",
                    "enum": ["1.1", "2"],
                    "description": "HTTP版本",
                    "default": "1.1"
                },
                "method": {
                    "type": "string",
                    "enum": ["GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS"],
                    "description": "HTTP方法",
                    "default": "POST"
                },
                "path": {
                    "type": "string",
                    "description": "请求路径",
                    "default": "/api/test"
                },
                "headers": {
                    "type": "object",
                    "description": "HTTP请求头",
                    "default": {
                        "Content-Type": "application/octet-stream"
                    }
                },
                "debug": {
                    "type": "object",
                    "properties": {
                        "enabled": {
                            "type": "boolean",
                            "description": "是否启用debug功能",
                            "default": false
                        },
                        "config": {
                            "type": "object",
                            "description": "Debug配置，同4.4.7节",
                            "default": {}
                        }
                    },
                    "default": {
                        "enabled": false,
                        "config": {}
                    }
                }
            },
            "required": ["http_version", "method", "path"]
        },
        "payload": {
            "type": "object",
            "properties": {
                "type": {
                    "type": "string",
                    "enum": ["file", "hex", "base64", "random"],
                    "description": "payload类型",
                    "default": "file"
                },
                "value": {
                    "type": "string",
                    "description": "payload值：文件路径/hex字符串/base64字符串/random字节数",
                    "default": ""
                },
                "random_length": {
                    "type": "integer",
                    "minimum": 0,
                    "maximum": 67108864,
                    "description": "随机payload的字节数，type=random时必填",
                    "default": 1024
                }
            },
            "required": ["type", "value"]
        },
        "scenario": {
            "type": "object",
            "properties": {
                "concurrent_clients": {
                    "type": "integer",
                    "minimum": 1,
                    "maximum": 1000,
                    "description": "并发client数量",
                    "default": 10
                },
                "requests_per_client": {
                    "type": "integer",
                    "minimum": 1,
                    "description": "每个client发送的请求数",
                    "default": 100
                },
                "keep_alive": {
                    "type": "boolean",
                    "description": "是否使用HTTP keep-alive",
                    "default": true
                },
                "delay_between_requests_ms": {
                    "type": "integer",
                    "minimum": 0,
                    "description": "请求间延迟（毫秒）",
                    "default": 0
                }
            },
            "required": ["concurrent_clients", "requests_per_client"]
        }
    },
    "required": ["server", "client", "payload", "scenario"]
}
```

---

## 5. 非功能规格详细设计

### 5.1 平台支持规格

| 规格ID | 规格项 | 详细规格 |
|-------|--------|---------|
| NFS-5.1.1 | 操作系统 | Server端支持Windows、Linux、Mac |
| NFS-5.1.2 | CPU架构 | 支持x86、x86_64、arm、arm64 |

#### 5.1.3 多平台构建规格

**各平台编译器要求**：

| 平台 | 编译器 | 最低版本 |
|-----|-------|---------|
| Windows | MSVC | Visual Studio 2019 (MSVC 19.20) |
| Linux | GCC | GCC 8.0 |
| Linux | Clang | Clang 7.0 |
| Mac | Clang | Xcode 10.0 (Clang 10.0) |

**构建工具链**：
- CMake 3.15或更高版本
- Python 3.8或更高版本（适配层）

**依赖安装方式**：
- Windows: 无特殊要求，所有三方库源码编译
- Linux: 需要安装基础构建工具（build-essential）
- Mac: 需要安装Xcode Command Line Tools

### 5.2 性能规格

| 规格ID | 规格项 | 详细规格 |
|-------|--------|---------|
| NFS-5.2.1 | 并发能力 | 高效、快速处理1000并发连接；<br>1000并发连接下平均响应延迟：≤100ms（p99延迟≤500ms）；<br>1000并发连接下吞吐量：≥10000请求/秒；<br>基准条件：1KB payload，局域网环境 |

### 5.3 构建与依赖规格

| 规格ID | 规格项 | 详细规格 |
|-------|--------|---------|
| NFS-5.3.1 | 三方库管理 | 所有三方库源码克隆到third_party，源码编译 |
| NFS-5.3.2 | 构建系统 | 使用CMake构建 |

### 5.4 错误处理规格

**错误处理策略**：

| 错误类型 | 处理策略 | 上报方式 |
|---------|---------|---------|
| 配置错误 | 终止启动，输出错误信息 | 标准错误输出 + 返回码 |
| 证书错误 | 终止启动，输出错误信息 | 标准错误输出 + 返回码 |
| 网络错误 | 记录日志，关闭对应连接 | 日志系统 |
| TLS错误 | 记录日志，关闭对应连接 | 日志系统 |
| 回调错误 | 记录日志，返回500错误 | 日志系统 + HTTP响应 |

**返回码定义**：

| 返回码 | 含义 |
|-------|------|
| 0 | 成功 |
| 1 | 配置错误 |
| 2 | 证书错误 |
| 3 | 网络错误 |
| 4 | 内部错误 |

### 5.5 日志格式规格

**日志级别**：

| 级别 | 说明 |
|-----|------|
| DEBUG | 调试信息 |
| INFO | 一般信息 |
| WARN | 警告信息 |
| ERROR | 错误信息 |

**日志格式**：

```
[时间戳] [级别] [模块] 消息内容
```

示例：
```
[2026-02-15 10:30:45.123] [INFO] [server] Server started on 0.0.0.0:8443
[2026-02-15 10:30:45.124] [DEBUG] [connection] New connection from 192.168.1.100:54321
```

**日志输出方式**：
- 默认输出到标准输出
- 可配置输出到文件
- 可配置日志级别

---

## 6. 接口规格设计

### 6.1 C接口回调定义

```c
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Client信息结构体
typedef struct {
    uint64_t connection_id;    // 连接ID
    const char* client_ip;     // client IP地址
    uint16_t client_port;      // client端口
    uint16_t server_port;      // server监听端口（用于策略选择）
    const char* token;         // token（预留，暂不使用）
    // 可扩展字段
} ClientContext;

/**
 * @brief 内容解析函数（异步）
 * @param ctx client上下文信息，生命周期见6.2节
 * @param in 输入报文二进制数据，生命周期见6.2节
 * @param inLen 输入报文长度
 * @return 错误码，0表示成功，见6.3节错误码定义
 *
 * 调用约定：
 * - 调用线程：工作线程（T4或T5）
 * - 并发调用：可被多个线程并发调用，需保证线程安全
 * - 阻塞行为：允许阻塞，但建议异步处理
 */
uint32_t AsyncParseContent(const ClientContext* ctx, const uint8_t* in, uint32_t inLen);

/**
 * @brief 内容回复函数（异步输出）
 * @param ctx client上下文信息，生命周期见6.2节
 * @param out 输出报文缓冲区，生命周期见6.2节
 * @param outLen 输入时为缓冲区大小，输出时为实际数据长度
 * @return 错误码，0表示成功，见6.3节错误码定义
 *
 * 调用约定：
 * - 调用线程：工作线程（T4或T5）
 * - 并发调用：可被多个线程并发调用，需保证线程安全
 * - 阻塞行为：允许阻塞
 */
uint32_t AsyncReplyContent(const ClientContext* ctx, uint8_t* out, uint32_t* outLen);

#ifdef __cplusplus
}
#endif
```

### 6.2 接口调用约定与生命周期管理

#### 6.2.1 调用时序

```
msg_center → message_parser_service: message
    activate message_parser_service
    message_parser_service → message_parser_task: create task with AsyncParseContent
        activate message_parser_task
    message_parser_service → msg_center: create task result
    deactivate message_parser_service
message_parser_task → message_parser_task: AsyncParseContent()
message_parser_task → msg_center: return with AsyncReplyContent()
    activate msg_center
msg_center → message_parser_service: finish task
    deactivate msg_center
    activate message_parser_service
message_parser_service → message_parser_task: delete task
    deactivate message_parser_task
    deactivate message_parser_service
```

#### 6.2.2 内存生命周期规则

| 参数 | 所属函数 | 生命周期 | 释放者 |
|-----|---------|---------|-------|
| ClientContext* ctx | AsyncParseContent | 调用期间有效，调用后失效 | 调用方 |
| const uint8_t* in | AsyncParseContent | 调用期间有效，调用后失效 | 调用方 |
| ClientContext* ctx | AsyncReplyContent | 调用期间有效，调用后失效 | 调用方 |
| uint8_t* out | AsyncReplyContent | 调用前由调用方分配，调用后由调用方释放 | 调用方 |
| uint32_t* outLen | AsyncReplyContent | 调用期间有效，调用后失效 | 调用方 |

#### 6.2.3 数据传递机制

1. **请求数据传递**：
   - msg_center接收到完整请求报文后，创建message_parser_task
   - 将报文数据通过in参数传递给AsyncParseContent
   - AsyncParseContent可保存报文数据供后续AsyncReplyContent使用

2. **响应数据传递**：
   - AsyncReplyContent被调用时，out缓冲区已分配
   - 回调实现将响应数据写入out缓冲区
   - 通过outLen返回实际数据长度
   - 最大响应数据长度：64MB

### 6.3 错误码定义

| 错误码 | 值 | 含义 | 处理方式 |
|-------|---|------|---------|
| SUCCESS | 0 | 成功 | 继续执行 |
| ERR_INVALID_PARAM | 1 | 参数无效 | 记录日志，返回400错误 |
| ERR_PARSE_FAILED | 2 | 解析失败 | 记录日志，返回400错误 |
| ERR_INTERNAL | 3 | 内部错误 | 记录日志，返回500错误 |
| ERR_BUSY | 4 | 系统忙 | 记录日志，返回503错误 |
| ERR_NOT_FOUND | 5 | 资源未找到 | 记录日志，返回404错误 |

---

## 7. 配置规格设计

### 7.1 JSON配置文件结构

```json
{
    "server": {
        "listen_ip": "0.0.0.0",
        "listen_port": 8443,
        "tls_version": "1.2|1.3|both",
        "thread_count": 5,
        "timeout_seconds": 30
    },
    "certificates": {
        "cert_file": "/path/to/cert.pem",
        "key_file": "/path/to/key.pem",
        "cert_type": "international|sm2|custom"
    },
    "debug": {
        "enabled": true,
        "log_packets": false
    },
    "callbacks": {
        "strategy": "port_based",
        "port_mapping": {
            "8443": "callback_1",
            "8444": "callback_2"
        }
    },
    "logging": {
        "level": "INFO",
        "file": "/path/to/server.log",
        "max_size_mb": 100,
        "max_files": 10
    }
}
```

### 7.2 配置项说明与校验规则

| 配置项 | 说明 | 是否必填 | 默认值 | 取值范围 | 校验规则 |
|--------|------|----------|--------|---------|---------|
| server.listen_ip | 监听IP地址 | 是 | - | 有效的IPv4或IPv6地址 | 必须是有效的IP地址格式 |
| server.listen_port | 监听端口 | 是 | - | 1-65535 | 必须是整数，在1-65535范围内 |
| server.tls_version | TLS版本 | 是 | - | "1.2", "1.3", "both" | 必须是指定的枚举值之一 |
| server.thread_count | 线程数 | 否 | 5 | 1-5 | 必须是整数，最大5 |
| server.timeout_seconds | 超时时间（秒） | 否 | 30 | 1-3600 | 必须是整数，在1-3600范围内 |
| certificates.cert_file | 证书文件路径 | 是 | - | 有效的文件路径 | 文件必须存在且可读 |
| certificates.key_file | 私钥文件路径 | 是 | - | 有效的文件路径 | 文件必须存在且可读 |
| certificates.cert_type | 证书类型 | 是 | - | "international", "sm2", "custom" | 必须是指定的枚举值之一；<br>international：标准X.509证书；<br>sm2：国密SM2证书；<br>custom：用户自定义证书 |
| debug.enabled | 是否启用debug | 否 | false | true/false | 布尔值 |
| debug.log_packets | 是否记录报文日志 | 否 | false | true/false | 布尔值 |
| callbacks.strategy | 回调策略 | 否 | "port_based" | "port_based" | 当前仅支持port_based |
| callbacks.port_mapping | 端口映射 | 否 | {} | JSON对象 | key为端口字符串，value为回调名称 |
| logging.level | 日志级别 | 否 | "INFO" | "DEBUG", "INFO", "WARN", "ERROR" | 必须是指定的枚举值之一 |
| logging.file | 日志文件路径 | 否 | "" | 有效的文件路径 | 若为空则输出到标准输出 |
| logging.max_size_mb | 单个日志文件最大大小（MB） | 否 | 100 | 1-1024 | 必须是整数，在1-1024范围内 |
| logging.max_files | 保留的日志文件数 | 否 | 10 | 1-100 | 必须是整数，在1-100范围内 |

### 7.3 配置文件校验与错误处理

**校验规则**：
1. 启动时先进行配置文件语法校验（JSON格式）
2. 然后进行配置项取值范围校验
3. 最后进行文件存在性校验（证书文件等）

**错误处理**：
- 校验失败时，输出详细错误信息，终止启动
- 错误信息包含：错误配置项、错误原因、合法取值范围

---

## 8. 项目目录结构规格

### 8.1 目录结构定义

```
https-server-sim/
├── docs/
│   └── .ai/
│       ├── srs-1-requirements.md       # SRS需求文档
│       └── srs/
│           └── account_manager/         # 需求沟通临时文件
├── codes/
│   ├── api/
│   │   ├── adapt/                       # 将core封装成可执行文件
│   │   │   ├── include/                 # 适配层头文件
│   │   │   └── source/                  # 适配层源文件
│   │   ├── cli/                         # Python CLI适配层
│   │   │   ├── __init__.py
│   │   │   ├── main.py                  # CLI入口
│   │   │   ├── config.py                # 配置解析
│   │   │   └── server.py                # Server控制
│   │   ├── app/                         # Python GUI适配层
│   │   │   ├── __init__.py
│   │   │   ├── main.py                  # GUI入口
│   │   │   ├── main_window.py           # 主窗口
│   │   │   ├── config_panel.py          # 配置面板
│   │   │   └── log_panel.py             # 日志面板
│   │   └── Test/                        # 测试代码
│   │       ├── unit_tests/              # 单元测试
│   │       └── integration_tests/       # 集成测试
│   └── core/
│       ├── include/                     # 头文件
│       │   ├── server/                  # Server模块头文件
│       │   ├── connection/              # 连接管理头文件
│       │   ├── protocol/                # 协议处理头文件
│       │   ├── msg_center/              # 消息中心头文件
│       │   ├── debug_chain/             # 调测职责链头文件
│       │   ├── callback/                # 回调机制头文件
│       │   └── utils/                   # 工具类头文件
│       ├── source/                      # 源文件
│       │   ├── server/                  # Server模块源文件
│       │   ├── connection/              # 连接管理源文件
│       │   ├── protocol/                # 协议处理源文件
│       │   ├── msg_center/              # 消息中心源文件
│       │   ├── debug_chain/             # 调测职责链源文件
│       │   ├── callback/                # 回调机制源文件
│       │   └── utils/                   # 工具类源文件
│       └── test/
│           └── sim_client/              # 测试用client模拟器
│               ├── __init__.py
│               ├── main.py              # Client入口
│               ├── config.py            # 配置解析
│               ├── client.py            # Client实现
│               └── scenario.py          # 测试场景
├── prompts/                             # 用户维护
├── scripts/
│   ├── build_project.py                 # 构建脚本
│   ├── server_sim_cli.py                # CLI入口
│   └── server_sim_app.py                # GUI入口
├── third_party/                         # 三方库源码
│   ├── openssl/                         # TLS库
│   ├── json/                            # JSON解析库
│   └── ...
└── CMakeLists.txt                       # CMake构建文件
```

### 8.2 文件命名规范

| 类型 | 命名规范 | 示例 |
|-----|---------|------|
| C++头文件 | snake_case，.hpp或.h | server.hpp, connection_manager.h |
| C++源文件 | snake_case，.cpp | server.cpp, connection_manager.cpp |
| Python文件 | snake_case，.py | main.py, config_panel.py |
| CMake文件 | 首字母大写，无后缀 | CMakeLists.txt |
| 配置文件 | snake_case，.json | server_config.json |

---

## 9. 验收标准定义

### 9.1 功能验收标准

| ID | 验收项 | 验收方法 | 判定标准 |
|----|--------|---------|---------|
| AC-9.1.1 | TLS 1.2连接 | 使用支持TLS 1.2的Client连接Server | 握手成功率100%，能够正常收发报文 |
| AC-9.1.2 | TLS 1.3连接 | 使用支持TLS 1.3的Client连接Server | 握手成功率100%，能够正常收发报文 |
| AC-9.1.3 | 1000并发连接 | 使用测试Client发起1000并发连接 | 所有连接建立成功，无连接被拒绝 |
| AC-9.1.4 | 回调机制验证 | 实现一个简单的echo回调，验证请求响应 | 回调被正确调用，响应数据正确返回 |
| AC-9.1.5 | 延迟响应调测 | 配置delay_ms=1000，发送请求 | 响应延迟在990-1010ms范围内 |
| AC-9.1.6 | 强制断开调测 | 配置force_disconnect=true，发送请求 | 响应返回后连接立即断开 |
| AC-9.1.7 | 错误码返回调测 | 配置http_status=404，发送请求 | 响应状态码为404 |
| AC-9.1.8 | 报文日志调测 | 配置log_packet=true，发送请求 | 请求和响应报文被记录到日志 |
| AC-9.1.9 | 多端口回调策略 | 配置8443和8444端口分别对应不同回调 | 不同端口的请求被路由到对应的回调 |
| AC-9.1.10 | 连接超时 | 设置timeout_seconds=5，建立连接后不发送数据 | 5秒后连接自动断开 |
| AC-9.1.11 | 国密证书支持 | 使用国密SM2证书启动Server，Client使用国密连接 | 握手成功，能够正常收发报文 |
| AC-9.1.12 | HTTP/2支持 | 使用HTTP/2 Client连接Server | 连接成功，能够正常收发报文 |

### 9.2 非功能验收标准

| ID | 验收项 | 验收方法 | 判定标准 |
|----|--------|---------|---------|
| AC-9.2.1 | Windows平台支持 | 在Windows 10/11上编译并运行 | 编译成功，功能验收通过 |
| AC-9.2.2 | Linux平台支持 | 在Ubuntu 20.04/22.04上编译并运行 | 编译成功，功能验收通过 |
| AC-9.2.3 | Mac平台支持 | 在macOS 11/12/13上编译并运行 | 编译成功，功能验收通过 |
| AC-9.2.4 | 1000并发性能 | 1000并发连接，1KB payload | 平均响应延迟≤100ms，p99延迟≤500ms，吞吐量≥10000请求/秒 |
| AC-9.2.5 | 内存占用 | 1000并发连接空闲状态 | 内存占用≤512MB |
| AC-9.2.6 | CPU占用 | 1000并发连接空闲状态 | CPU占用≤5% |
| AC-9.2.7 | 大报文支持 | 发送64MB大小的报文 | 传输成功，数据完整无误 |

### 9.3 配置验收标准

| ID | 验收项 | 验收方法 | 判定标准 |
|----|--------|---------|---------|
| AC-9.3.1 | 配置文件校验 | 使用无效配置项启动 | 正确检测错误，输出明确错误信息，终止启动 |
| AC-9.3.2 | 证书文件校验 | 使用不存在的证书文件启动 | 正确检测错误，输出明确错误信息，终止启动 |
| AC-9.3.3 | 端口范围校验 | 使用端口0或65536启动 | 正确检测错误，输出明确错误信息，终止启动 |

---

**文档结束**
