# HTTPS Server 模拟器 - 产品规格分析文档

**版本**: v1
**创建日期**: 2026-02-15
**状态**: 初始版本

---

## 目录

1. [产品定位分析](#1-产品定位分析)
2. [目标用户分析](#2-目标用户分析)
3. [功能规格详细设计](#3-功能规格详细设计)
4. [非功能规格详细设计](#4-非功能规格详细设计)
5. [接口规格设计](#5-接口规格设计)
6. [配置规格设计](#6-配置规格设计)
7. [验收标准定义](#7-验收标准定义)

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

## 3. 功能规格详细设计

### 3.1 HTTPS Server核心功能规格

| 功能ID | 功能名称 | 详细规格 |
|-------|---------|---------|
| FS-3.1.1 | TLS连接管理 | 支持TLS 1.2、TLS 1.3，可配置 |
| FS-3.1.2 | 证书管理 | 支持国际证书、国密SM2证书、自定义证书 |
| FS-3.1.3 | HTTP协议支持 | 支持HTTP/1.1、HTTP/2及所有版本 |
| FS-3.1.4 | HTTP方法支持 | 支持GET、POST、PUT、DELETE等所有HTTP方法 |
| FS-3.1.5 | 报文透传 | HTTPS body作为二进制流透传，不做解析 |
| FS-3.1.6 | 响应返回 | 从回调获取响应数据，原样返回给client |

### 3.2 并发与连接管理规格

| 功能ID | 功能名称 | 详细规格 |
|-------|---------|---------|
| FS-3.2.1 | 进程模型 | 单进程设计，最多1个进程 |
| FS-3.2.2 | 线程模型 | 进程内至多5个线程 |
| FS-3.2.3 | 连接容量 | 同时支持1000个client连接 |
| FS-3.2.4 | 连接建立判定 | TCP三次握手 + TLS认证成功 = 连接建立 |
| FS-3.2.5 | 连接超时 | 默认30秒无活动断开，可配置 |
| FS-3.2.6 | 连接断开处理 | 超时后直接关闭TCP连接 |
| FS-3.2.7 | 生命周期跟踪 | 跟踪client所有状态：已连接、握手完成、报文中、已断开等 |

### 3.3 C接口回调功能规格

| 功能ID | 功能名称 | 详细规格 |
|-------|---------|---------|
| FS-3.3.1 | 静态链接方式 | 回调通过CMake object静态链接 |
| FS-3.3.2 | 策略模式 | 支持多个回调object，通过server端口选择策略 |
| FS-3.3.3 | 异步处理 | 回调采用异步方式处理报文 |
| FS-3.3.4 | Event Loop | msg_center采用event loop机制 |

### 3.4 Debug调测功能规格

| 功能ID | 功能名称 | 详细规格 |
|-------|---------|---------|
| FS-3.4.1 | Debug字段解析 | 解析HTTP header末尾的`<debug-server-sim>`字段 |
| FS-3.4.2 | 延迟响应 | 支持配置延迟响应时间，单位毫秒 |
| FS-3.4.3 | 强制断开 | 支持强制断开client连接 |
| FS-3.4.4 | 报文日志 | 支持记录报文日志 |
| FS-3.4.5 | 错误码返回 | 支持返回特定HTTP错误码 |
| FS-3.4.6 | 职责链模式 | 调测点采用职责链模式，可叠加，支持扩展 |

### 3.5 适配层功能规格

| 功能ID | 功能名称 | 详细规格 |
|-------|---------|---------|
| FS-3.5.1 | CLI界面 | Python 3.8命令行界面 |
| FS-3.5.2 | GUI界面 | Python 3.8图形界面（Windows使用） |
| FS-3.5.3 | 配置文件 | JSON格式配置文件 |

### 3.6 测试用Client模拟器规格

| 功能ID | 功能名称 | 详细规格 |
|-------|---------|---------|
| FS-3.6.1 | 实现语言 | 全Python实现，不对外发布 |
| FS-3.6.2 | HTTPS支持 | 支持HTTPS，可配置证书 |
| FS-3.6.3 | HTTP版本 | 支持HTTP/1.1和HTTP/2 |
| FS-3.6.4 | 报文配置 | 通过JSON Schema配置文件生成报文 |
| FS-3.6.5 | 连接管理 | 支持建联、发送报文、接收响应 |

---

## 4. 非功能规格详细设计

### 4.1 平台支持规格

| 规格ID | 规格项 | 详细规格 |
|-------|--------|---------|
| NFS-4.1.1 | 操作系统 | Server端支持Windows、Linux、Mac |
| NFS-4.1.2 | CPU架构 | 支持x86、x86_64、arm、arm64 |

### 4.2 性能规格

| 规格ID | 规格项 | 详细规格 |
|-------|--------|---------|
| NFS-4.2.1 | 并发能力 | 高效、快速处理1000并发连接 |

### 4.3 构建与依赖规格

| 规格ID | 规格项 | 详细规格 |
|-------|--------|---------|
| NFS-4.3.1 | 三方库管理 | 所有三方库源码克隆到third_party，源码编译 |
| NFS-4.3.2 | 构建系统 | 使用CMake构建 |

---

## 5. 接口规格设计

### 5.1 C接口回调定义

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
 * @param ctx client上下文信息
 * @param in 输入报文二进制数据
 * @param inLen 输入报文长度
 * @return 错误码，0表示成功
 */
uint32_t AsyncParseContent(const ClientContext* ctx, const uint8_t* in, uint32_t inLen);

/**
 * @brief 内容回复函数（异步输出）
 * @param ctx client上下文信息
 * @param out 输出报文缓冲区
 * @param outLen 输入时为缓冲区大小，输出时为实际数据长度
 * @return 错误码，0表示成功
 */
uint32_t AsyncReplyContent(const ClientContext* ctx, uint8_t* out, uint32_t* outLen);

#ifdef __cplusplus
}
#endif
```

### 5.2 回调调用时序

```
msg_center → message_parser_service: message
    activate message_parser_service
    message_parser_service → message_parser_task: create task with AsyncParseContent
        activate message_parser_task
    message_parser_service → msg_center: create task result
    deactivate message_parser_service
message_parser_task → message_parser_task: ParseContent
message_parser_task → msg_center: return with AsyncReplyContent
    activate msg_center
msg_center → message_parser_service: finish task
    deactivate msg_center
    activate message_parser_service
message_parser_service → message_parser_task: delete task
    deactivate message_parser_task
    deactivate message_parser_service
```

---

## 6. 配置规格设计

### 6.1 JSON配置文件结构

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
    }
}
```

### 6.2 配置项说明

| 配置项 | 说明 | 是否必填 | 默认值 |
|--------|------|----------|--------|
| server.listen_ip | 监听IP地址 | 是 | - |
| server.listen_port | 监听端口 | 是 | - |
| server.tls_version | TLS版本 | 是 | - |
| server.thread_count | 线程数（最大5） | 否 | 5 |
| server.timeout_seconds | 超时时间（秒） | 否 | 30 |
| certificates.cert_file | 证书文件路径 | 是 | - |
| certificates.key_file | 私钥文件路径 | 是 | - |
| certificates.cert_type | 证书类型 | 是 | - |
| debug.enabled | 是否启用debug | 否 | false |
| debug.log_packets | 是否记录报文日志 | 否 | false |

---

## 7. 验收标准定义

### 7.1 功能验收标准

| ID | 验收项 | 验收方法 |
|----|--------|---------|
| AC-7.1.1 | TLS 1.2/1.3连接 | 使用支持TLS 1.2和1.3的Client分别连接验证 |
| AC-7.1.2 | 1000并发连接 | 使用测试Client发起1000并发连接验证 |
| AC-7.1.3 | 回调机制 | 验证回调函数能正确接收请求并返回响应 |
| AC-7.1.4 | 调测功能 | 验证各调测点配置生效 |

### 7.2 非功能验收标准

| ID | 验收项 | 验收方法 |
|----|--------|---------|
| AC-7.2.1 | 多平台支持 | 在Windows、Linux、Mac上分别验证 |
| AC-7.2.2 | 性能 | 验证1000并发下的响应延迟 |

---

**文档结束**
