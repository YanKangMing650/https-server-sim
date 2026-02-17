# HTTPS Server 模拟器 - 架构设计文档 (HLD)

**版本**: v4
**创建日期**: 2026-02-15
**修改日期**: 2026-02-16
**状态**: 草稿

---

## 目录

1. [文档概述](#1-文档概述)
2. [系统架构总览](#2-系统架构总览)
3. [核心模块设计](#3-核心模块设计)
4. [接口设计](#4-接口设计)
5. [数据结构设计](#5-数据结构设计)
6. [线程与并发设计](#6-线程与并发设计)
7. [数据流设计](#7-数据流设计)
8. [配置设计](#8-配置设计)
9. [错误处理设计](#9-错误处理设计)
10. [日志设计](#10-日志设计)
11. [目录结构与构建设计](#11-目录结构与构建设计)
12. [可扩展性设计](#12-可扩展性设计)
13. [防腐层设计](#13-防腐层设计)
14. [引用文档](#14-引用文档)

---

## 1. 文档概述

### 1.1 版本历史

| 版本 | 日期 | 作者 | 说明 |
|------|------|------|
| 1.0 | 2026-02-15 | Architecture Designer | 初始版本 |
| v2 | 2026-02-16 | Architecture Designer | 根据检视意见修改：移除疑问点、补充IO线程设计、国密配置、回调超时、大报文处理、Graceful Shutdown、职责边界、配置职责、协议栈层次、命名统一、配置校验规则、状态机澄清、EventQueue实现、回调命名、模块交互视图、扩展点设计 |
| v3 | 2026-02-16 | Architecture Designer | 修复格式问题：补全模块列表表格、闭合括号 |
| v4 | 2026-02-16 | Architecture Designer | 新增：4+1视图、子模块文档、工程设计、防腐层设计；重构接口设计确保所有接口被类接管 |

### 1.2 参考文档

| 文档名称 | 路径 |
|---------|------|
| 需求产品规格分析文档 | /Users/kangbenben/Documents/codes/ai-code/YanKangMing650/https-server-sim/docs/design/srs-需求分析文档.md |
| 需求获取文档 | /Users/kangbenben/Documents/codes/ai-code/YanKangMing650/https-server-sim/docs/design/srs-需求文档.md |

### 1.3 文档状态

本文档基于需求文档撰写，当前状态为**草稿**，待评审后更新为正式版本。

---

## 2. 系统架构总览

### 2.1 分层架构图

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

### 2.2 各层职责

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

---

## 3. 核心模块设计

### 3.1 模块列表

| 模块名称 | 模块路径 | 职责描述 | 详细设计文档 |
|---------|---------|---------|-------------|
| Server | codes/core/source/server/ | Server生命周期管理，监听socket管理，Graceful Shutdown | [module-server.md](modules/module-server.md) |
| Connection | codes/core/source/connection/ | 连接生命周期管理，状态机，超时处理 | [module-connection.md](modules/module-connection.md) |
| Protocol | codes/core/source/protocol/ | TLS握手，HTTP/1.1、HTTP/2协议解析，国密支持 | [module-protocol.md](modules/module-protocol.md) |
| MsgCenter | codes/core/source/msg_center/ | Event Loop，事件队列，IO线程，WorkerPool | [module-msgcenter.md](modules/module-msgcenter.md) |
| DebugChain | codes/core/source/debug_chain/ | 调测点职责链（延迟、断开、日志、错误码） | [module-debugchain.md](modules/module-debugchain.md) |
| Callback | codes/core/source/callback/ | 回调策略管理，端口映射 | [module-callback.md](modules/module-callback.md) |
| Utils | codes/core/source/utils/ | 日志、无锁队列、缓冲区、统计、配置解析 | [module-utils.md](modules/module-utils.md) |
| Adapt | codes/api/adapt/ | C接口封装，供Python调用 | 见第4节接口设计 |

### 3.2 Server模块

详见 [module-server.md](modules/module-server.md)

### 3.3 Connection模块

详见 [module-connection.md](modules/module-connection.md)

### 3.4 Protocol模块

详见 [module-protocol.md](modules/module-protocol.md)

### 3.5 MsgCenter模块

详见 [module-msgcenter.md](modules/module-msgcenter.md)

### 3.6 DebugChain模块

详见 [module-debugchain.md](modules/module-debugchain.md)

### 3.7 Callback模块

详见 [module-callback.md](modules/module-callback.md)

### 3.8 Utils模块

详见 [module-utils.md](modules/module-utils.md)

### 3.9 Adapt模块

详见第4节接口设计

---

## 4. 接口设计

### 4.1 C接口（供Python调用）- 由ServerAdapt类接管

所有C接口均由ServerAdapt类统一管理，确保接口被类接管。

**文件路径**: codes/api/adapt/include/server_adapt.h

```c
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Server状态枚举
typedef enum {
    SERVER_STATUS_STOPPED = 0,
    SERVER_STATUS_INITIALIZING = 1,
    SERVER_STATUS_RUNNING = 2,
    SERVER_STATUS_SHUTTING_DOWN = 3,
    SERVER_STATUS_ERROR = 4
} ServerStatusEnum;

// Server状态结构体
typedef struct {
    ServerStatusEnum status;
    uint64_t uptime_seconds;
    uint32_t current_connections;
    uint16_t listen_port;
    char listen_ip[64];
} ServerStatus;

// 统计信息结构体
typedef struct {
    uint64_t total_connections;
    uint64_t total_requests;
    uint64_t requests_per_second;
    uint32_t avg_response_latency_ms;
    uint32_t p50_response_latency_ms;
    uint32_t p95_response_latency_ms;
    uint32_t p99_response_latency_ms;
    uint64_t total_bytes_received;
    uint64_t total_bytes_sent;
} Statistics;

// ServerAdapt类前置声明
typedef struct ServerAdapt ServerAdapt;

// 创建ServerAdapt实例
ServerAdapt* server_adapt_create(void);

// 销毁ServerAdapt实例
void server_adapt_destroy(ServerAdapt* adapt);

// Server初始化
// @param adapt ServerAdapt实例
// @param config_file 配置文件路径
// @return 0表示成功，非0表示错误码
int server_adapt_init(ServerAdapt* adapt, const char* config_file);

// Server启动
// @param adapt ServerAdapt实例
// @return 0表示成功，非0表示错误码
int server_adapt_start(ServerAdapt* adapt);

// Server停止
// @param adapt ServerAdapt实例
// @return 0表示成功，非0表示错误码
int server_adapt_stop(ServerAdapt* adapt);

// Server状态查询
// @param adapt ServerAdapt实例
// @param status 输出参数，Server状态
// @return 0表示成功，非0表示错误码
int server_adapt_get_status(ServerAdapt* adapt, ServerStatus* status);

// 获取统计信息
// @param adapt ServerAdapt实例
// @param stats 输出参数，统计信息
// @return 0表示成功，非0表示错误码
int server_adapt_get_statistics(ServerAdapt* adapt, Statistics* stats);

// Server清理
// @param adapt ServerAdapt实例
void server_adapt_cleanup(ServerAdapt* adapt);

#ifdef __cplusplus
}
#endif
```

### 4.2 C++ ServerAdapt类定义

**文件路径**: codes/api/adapt/include/server_adapt.hpp

```cpp
#pragma once

#include <string>
#include <memory>
#include "server_adapt.h"
#include "server/server.hpp"

namespace https_server_sim {

class ServerAdapt {
public:
    ServerAdapt();
    ~ServerAdapt();

    // 初始化Server，加载配置
    int init(const std::string& config_file);

    // 启动Server
    int start();

    // 停止Server
    int stop();

    // 获取状态
    void get_status(ServerStatus* status) const;

    // 获取统计信息
    void get_statistics(Statistics* stats) const;

    // 清理资源
    void cleanup();

private:
    std::unique_ptr<Server> server_;
};

} // namespace https_server_sim
```

### 4.3 C接口回调定义 - 由CallbackRegistry类接管

所有回调接口均由CallbackRegistry类统一管理。

**文件路径**: codes/core/include/callback/callback.h

```c
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Client信息结构体
typedef struct {
    uint64_t connection_id;
    const char* client_ip;
    uint16_t client_port;
    uint16_t server_port;
    const char* token;
} ClientContext;

// 回调函数类型定义
typedef uint32_t (*AsyncParseContentFunc)(const ClientContext* ctx,
                                           const uint8_t* in,
                                           uint32_t inLen);

typedef uint32_t (*AsyncReplyContentFunc)(const ClientContext* ctx,
                                           uint8_t* out,
                                           uint32_t* outLen);

// 回调策略结构
typedef struct {
    const char* name;
    uint16_t port;
    AsyncParseContentFunc parse;
    AsyncReplyContentFunc reply;
} CallbackStrategy;

// CallbackRegistry类前置声明
typedef struct CallbackRegistry CallbackRegistry;

// 创建CallbackRegistry实例
CallbackRegistry* callback_registry_create(void);

// 销毁CallbackRegistry实例
void callback_registry_destroy(CallbackRegistry* registry);

// 注册回调策略
// @param registry CallbackRegistry实例
// @param strategy 回调策略
// @return 0表示成功，非0表示错误码
int callback_registry_register_strategy(CallbackRegistry* registry,
                                        const CallbackStrategy* strategy);

// 获取指定端口的回调策略
// @param registry CallbackRegistry实例
// @param port 端口号
// @return 回调策略指针，失败返回NULL
const CallbackStrategy* callback_registry_get_strategy(CallbackRegistry* registry,
                                                        uint16_t port);

#ifdef __cplusplus
}
#endif
```

### 4.4 调测点扩展接口 - 由DebugHandlerRegistry类接管

所有调测点接口均由DebugHandlerRegistry类统一管理。

**文件路径**: codes/core/include/debug_chain/debug_handler.h

```c
#pragma once

#include <stdint.h>
#include "callback/client_context.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DebugHandler DebugHandler;
typedef struct DebugConfig DebugConfig;
typedef struct DebugContext DebugContext;

struct DebugHandler {
    const char* name;
    int priority;
    void* user_data;

    int (*handle_request)(DebugHandler* self,
                         const ClientContext* ctx,
                         const DebugConfig* config,
                         DebugContext* debug_ctx);

    int (*handle_response)(DebugHandler* self,
                          const ClientContext* ctx,
                          const DebugConfig* config,
                          DebugContext* debug_ctx);

    void (*destroy)(DebugHandler* self);
};

// DebugHandlerRegistry类前置声明
typedef struct DebugHandlerRegistry DebugHandlerRegistry;

// 创建DebugHandlerRegistry实例
DebugHandlerRegistry* debug_handler_registry_create(void);

// 销毁DebugHandlerRegistry实例
void debug_handler_registry_destroy(DebugHandlerRegistry* registry);

// 注册调测点
// @param registry DebugHandlerRegistry实例
// @param handler 调测点处理器
// @return 0表示成功，非0表示错误码
int debug_handler_registry_register(DebugHandlerRegistry* registry,
                                     DebugHandler* handler);

// 注销调测点
// @param registry DebugHandlerRegistry实例
// @param name 调测点名称
// @return 0表示成功，非0表示错误码
int debug_handler_registry_unregister(DebugHandlerRegistry* registry,
                                       const char* name);

#ifdef __cplusplus
}
#endif
```

### 4.5 内部模块接口

详见各子模块详细设计文档。

---

## 5. 数据结构设计

详见各子模块详细设计文档。

---

## 6. 线程与并发设计

详见 [module-msgcenter.md](modules/module-msgcenter.md)

---

## 7. 数据流设计

详见 [hld-4+1视图.md](hld-4+1视图.md) 中的过程视图。

---

## 8. 配置设计

详见 [module-utils.md](modules/module-utils.md)

---

## 9. 错误处理设计

详见各子模块详细设计文档。

---

## 10. 日志设计

详见 [module-utils.md](modules/module-utils.md)

---

## 11. 目录结构与构建设计

详见 [hld-工程设计.md](hld-工程设计.md)

---

## 12. 可扩展性设计

### 12.1 扩展点设计原则

架构采用插件式扩展设计，通过标准化接口定义，支持后续功能的增减、替换。

### 12.2 明确扩展点列表

| 扩展点 | 接口定义位置 | 扩展方式 | 说明 |
|-------|---------|---------|------|
| 调测点扩展 | DebugHandler接口 | 实现DebugHandler接口，调用RegisterDebugHandler注册 | 可添加新的调测点，如：流量限制、流量统计、报文篡改等 |
| 协议扩展 | ProtocolHandler接口 | 实现ProtocolHandler接口，新增协议处理器 | 可添加新的协议版本，如HTTP/3 |
| 回调策略扩展 | CallbackStrategy接口 | 实现AsyncParseContent和AsyncReplyContent函数 | 用户自定义报文处理逻辑 |
| 证书类型扩展 | TlsHandler::configure_certificates() | 扩展configure_certificates() | 可添加新的证书类型支持 |
| 网络IO平台扩展 | IoThread类 | 平台特定的event_loop_*()函数 | 支持新的平台的网络IO模型 |

### 12.3 模块扩展规范

详见各子模块详细设计文档。

---

## 13. 防腐层设计

### 13.1 防腐层设计目标

防腐层（Anticorruption Layer）的设计目标是：
- 隔离核心层与外部依赖
- 提供统一的接口，隐藏外部库的实现细节
- 便于后续替换外部依赖
- 降低核心层与外部库的耦合度

### 13.2 防腐层结构

```
┌─────────────────────────────────────────────────────────┐
│                    核心层 (Core)                         │
│  Server | Connection | Protocol | MsgCenter | ...      │
└─────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│                   防腐层 (Anticorruption Layer)         │
│  ┌──────────────────┐  ┌──────────────────┐          │
│  │  TlsHandler      │  │  Config          │          │
│  │  (OpenSSL封装)   │  │  (JSON封装)      │          │
│  └──────────────────┘  └──────────────────┘          │
└─────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│                   外部依赖 (External Dependencies)      │
│  ┌──────────────────┐  ┌──────────────────┐          │
│  │  OpenSSL         │  │  nlohmann/json   │          │
│  └──────────────────┘  └──────────────────┘          │
└─────────────────────────────────────────────────────────┘
```

### 13.3 TlsHandler 防腐层设计

详见 [module-protocol.md](modules/module-protocol.md)

### 13.4 Config 防腐层设计

详见 [module-utils.md](modules/module-utils.md)

### 13.5 防腐层扩展规范

详见 [hld-工程设计.md](hld-工程设计.md)

---

## 14. 引用文档

| 文档名称 | 路径 | 说明 |
|---------|------|------|
| 4+1视图设计 | [hld-4+1视图.md](hld-4+1视图.md) | 逻辑视图、物理视图、过程视图、开发视图、场景视图 |
| 工程设计 | [hld-工程设计.md](hld-工程设计.md) | 目录结构、构建、三方库、测试、防腐层详细设计 |
| Server模块详细设计 | [module-server.md](modules/module-server.md) | Server模块详细设计 |
| Connection模块详细设计 | [module-connection.md](modules/module-connection.md) | Connection模块详细设计 |
| Protocol模块详细设计 | [module-protocol.md](modules/module-protocol.md) | Protocol模块详细设计 |
| MsgCenter模块详细设计 | [module-msgcenter.md](modules/module-msgcenter.md) | MsgCenter模块详细设计 |
| DebugChain模块详细设计 | [module-debugchain.md](modules/module-debugchain.md) | DebugChain模块详细设计 |
| Callback模块详细设计 | [module-callback.md](modules/module-callback.md) | Callback模块详细设计 |
| Utils模块详细设计 | [module-utils.md](modules/module-utils.md) | Utils模块详细设计 |

---

**文档结束**
