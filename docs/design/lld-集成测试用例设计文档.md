# HTTPS Server 模拟器 - 集成测试用例设计文档

**版本**: v1
**创建日期**: 2026-02-21
**状态**: 草稿

---

## 目录

1. [文档概述](#1-文档概述)
2. [测试策略](#2-测试策略)
3. [测试环境](#3-测试环境)
4. [集成测试用例](#4-集成测试用例)
5. [性能测试用例](#5-性能测试用例)
6. [压力测试用例](#6-压力测试用例)
7. [测试工具](#7-测试工具)

---

## 1. 文档概述

### 1.1 测试目标

本文档定义 HTTPS Server 模拟器核心层（core）的集成测试用例，验证多个模块协同工作的正确性。

### 1.2 测试范围

| 测试类型 | 说明 |
|---------|------|
| 模块集成测试 | 验证 Server + Connection + Protocol + MsgCenter 等模块的协作 |
| 端到端测试 | 验证完整的 HTTPS 请求响应流程 |
| 调测功能测试 | 验证 DebugChain 的调测点功能 |
| 回调流程测试 | 验证 CallbackStrategy 的回调执行流程 |
| 并发测试 | 验证多连接并发场景 |
| 性能测试 | 验证 Server 的性能指标（延迟、吞吐量） |
| 压力测试 | 验证 Server 在高负载下的稳定性 |

---

## 2. 测试策略

### 2.1 测试架构

```
┌─────────────────────────────────────────────────────────┐
│              测试控制层 (Python)                        │
│  ┌──────────────────┐     ┌──────────────────┐       │
│  │  集成测试用例     │     │  SimClient       │       │
│  │  (GTest/C++)     │     │  (Python)        │       │
│  └──────────────────┘     └──────────────────┘       │
└─────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│              C++ 核心层 (core)                          │
│  Server + Connection + Protocol + MsgCenter + ...      │
└─────────────────────────────────────────────────────────┘
```

### 2.2 测试工具

| 工具 | 用途 | 位置 |
|------|------|------|
| GoogleTest | 集成测试框架 | third_party/googletest |
| SimClient | 模拟 HTTPS 客户端 | codes/core/test/sim_client |

---

## 3. 测试环境

### 3.1 测试配置

#### Server 配置模板

```json
{
    "listens": [
        {
            "ip": "127.0.0.1",
            "port": 19443,
            "enabled": true
        }
    ],
    "certificates": {
        "cert_path": "test_cert.pem",
        "key_path": "test_key.pem",
        "ca_path": "",
        "cipher_suite": "TLS_AES_256_GCM_SHA384"
    },
    "debug": {
        "enabled": true,
        "debug_points": []
    },
    "callbacks": {
        "callbacks_dir": "",
        "callbacks": []
    },
    "logging": {
        "level": "DEBUG",
        "file": "",
        "console_output": true
    },
    "http2": {
        "enabled": true,
        "allow_h2c": false
    }
}
```

#### SimClient 配置模板

```json
{
    "self": {
        "ip": "127.0.0.1",
        "port": 20001,
        "tls": {
            "enabled": true,
            "version": "1.2",
            "cert_path": "",
            "key_path": "",
            "ca_path": "",
            "cipher_suite": "TLS_AES_256_GCM_SHA384"
        },
        "auto_offline_seconds": 300
    },
    "target_server": {
        "ip": "127.0.0.1",
        "port": 19443
    },
    "messages": [
        {
            "request": {
                "method": "POST",
                "path": "/api/test",
                "headers": {
                    "Content-Type": "application/json"
                },
                "body": "{\"key\": \"value\"}"
            },
            "debug": {
                "force_error_code": 0,
                "response_timeout_ms": 5000
            }
        }
    ]
}
```

---

## 4. 集成测试用例

### 4.1 用例 1: Server 完整启动流程

**用例 ID**: IT-001

**测试名称**: Server 完整启动流程

**优先级**: 高

**前置条件**: 无

**测试步骤**:
1. 创建 Config 对象，加载有效配置
2. 创建 Server 对象
3. 调用 server.init(config_file)
4. 调用 server.start()
5. 查询 Server 状态
6. 调用 server.stop()
7. 调用 server.cleanup()

**预期结果**:
- init() 返回 0（成功）
- start() 返回 0（成功）
- 状态为 SERVER_STATUS_RUNNING
- stop() 返回 0（成功）
- cleanup() 成功执行

**涉及模块**: Server, Config, MsgCenter

**测试文件**: test_server_startup.cpp

---

### 4.2 用例 2: 单个 HTTPS 请求响应

**用例 ID**: IT-002

**测试名称**: 单个 HTTPS 请求响应

**优先级**: 高

**前置条件**: IT-001 执行成功

**测试步骤**:
1. 启动 Server（监听 19443 端口）
2. 启动 SimClient（绑定 20001 端口）
3. SimClient 发送 1 个 POST 请求
4. 等待响应
5. 停止 SimClient
6. 停止 Server

**预期结果**:
- Server 接受连接
- TLS 握手成功
- Server 收到请求
- Server 返回响应
- SimClient 收到响应
- 连接正常关闭

**涉及模块**: Server, Connection, Protocol, MsgCenter, DebugChain

**测试文件**: test_single_request.cpp

---

### 4.3 用例 3: 多个 HTTPS 请求（Keep-Alive）

**用例 ID**: IT-003

**测试名称**: 多个 HTTPS 请求（Keep-Alive）

**优先级**: 高

**前置条件**: IT-002 执行成功

**测试步骤**:
1. 启动 Server
2. 启动 SimClient
3. SimClient 发送 5 个连续请求（同一连接）
4. 等待所有响应
5. 停止 SimClient
6. 停止 Server

**预期结果**:
- 5 个请求都成功处理
- 5 个响应都正确返回
- 连接保持直到最后一个请求完成

**涉及模块**: Server, Connection, Protocol

**测试文件**: test_multiple_requests.cpp

---

### 4.4 用例 4: 并发连接测试

**用例 ID**: IT-004

**测试名称**: 并发连接测试

**优先级**: 高

**前置条件**: IT-002 执行成功

**测试步骤**:
1. 启动 Server
2. 同时启动 10 个 SimClient（不同端口：20001-20010）
3. 每个 SimClient 发送 1 个请求
4. 等待所有响应
5. 停止所有 SimClient
6. 停止 Server

**预期结果**:
- 10 个连接都成功建立
- 10 个请求都成功处理
- 10 个响应都正确返回
- ConnectionManager 正确管理所有连接

**涉及模块**: Server, ConnectionManager, MsgCenter

**测试文件**: test_concurrent_connections.cpp

---

### 4.5 用例 5: DebugChain 延迟响应调测点

**用例 ID**: IT-005

**测试名称**: DebugChain 延迟响应调测点

**优先级**: 中

**前置条件**: IT-002 执行成功

**测试步骤**:
1. 配置 Server 开启 DebugChain，启用 DelayHandler
2. 启动 Server
3. 启动 SimClient，添加 Header: X-Debug-Response-Timeout-Ms: 2000
4. SimClient 发送请求
5. 测量响应时间
6. 停止 SimClient 和 Server

**预期结果**:
- 响应延迟约 2000ms
- 响应正确返回

**涉及模块**: DebugChain, Protocol

**测试文件**: test_debug_delay.cpp

---

### 4.6 用例 6: DebugChain 强制错误码调测点

**用例 ID**: IT-006

**测试名称**: DebugChain 强制错误码调测点

**优先级**: 中

**前置条件**: IT-002 执行成功

**测试步骤**:
1. 配置 Server 开启 DebugChain，启用 ErrorCodeHandler
2. 启动 Server
3. 启动 SimClient，添加 Header: X-Debug-Force-Error-Code: 500
4. SimClient 发送请求
5. 检查响应状态码
6. 停止 SimClient 和 Server

**预期结果**:
- 响应 HTTP 状态码为 500

**涉及模块**: DebugChain, Protocol

**测试文件**: test_debug_error_code.cpp

---

### 4.7 用例 7: DebugChain 强制断开调测点

**用例 ID**: IT-007

**测试名称**: DebugChain 强制断开调测点

**优先级**: 中

**前置条件**: IT-002 执行成功

**测试步骤**:
1. 配置 Server 开启 DebugChain，启用 DisconnectHandler
2. 启动 Server
3. 启动 SimClient，设置强制断开
4. SimClient 发送请求
5. 观察连接状态
6. 停止 SimClient 和 Server

**预期结果**:
- Server 处理请求后立即断开连接

**涉及模块**: DebugChain, Connection

**测试文件**: test_debug_disconnect.cpp

---

### 4.8 用例 8: 回调策略执行流程

**用例 ID**: IT-008

**测试名称**: 回调策略执行流程

**优先级**: 高

**前置条件**: IT-002 执行成功

**测试步骤**:
1. 注册测试回调策略到端口 19443
2. 启动 Server
3. 启动 SimClient 连接到 19443
4. SimClient 发送请求
5. 验证回调函数被调用
6. 验证响应数据来自回调
7. 停止 SimClient 和 Server

**预期结果**:
- AsyncParseContent 被调用
- AsyncReplyContent 被调用
- 响应数据与回调返回一致

**涉及模块**: Callback, Server, Protocol

**测试文件**: test_callback_flow.cpp

---

### 4.9 用例 9: 多端口监听

**用例 ID**: IT-009

**测试名称**: 多端口监听

**优先级**: 中

**前置条件**: IT-001 执行成功

**测试步骤**:
1. 配置 Server 监听 19443 和 19444 两个端口
2. 启动 Server
3. SimClient1 连接到 19443 发送请求
4. SimClient2 连接到 19444 发送请求
5. 验证两个请求都成功
6. 停止 SimClient 和 Server

**预期结果**:
- 两个端口都成功监听
- 两个连接都成功建立
- 两个请求都成功处理

**涉及模块**: Server, ConnectionManager

**测试文件**: test_multi_port.cpp

---

### 4.10 用例 10: Graceful Shutdown

**用例 ID**: IT-010

**测试名称**: Graceful Shutdown

**优先级**: 高

**前置条件**: IT-004 执行成功

**测试步骤**:
1. 启动 Server
2. 启动 5 个 SimClient 发送长耗时请求
3. 在请求处理中调用 server.stop()
4. 等待 Server 完全停止
5. 检查所有连接状态

**预期结果**:
- Server 停止接受新连接
- 现有请求处理完成
- 所有连接正常关闭
- Server 资源正确清理

**涉及模块**: Server, ConnectionManager, MsgCenter

**测试文件**: test_graceful_shutdown.cpp

---

### 4.11 用例 11: HTTP/2 协议支持

**用例 ID**: IT-011

**测试名称**: HTTP/2 协议支持

**优先级**: 中

**前置条件**: IT-002 执行成功

**测试步骤**:
1. 配置 Server 启用 HTTP/2
2. 启动 Server
3. SimClient 使用 HTTP/2 发送请求
4. 验证响应
5. 停止 SimClient 和 Server

**预期结果**:
- ALPN 协商成功选择 h2
- HTTP/2 帧正确处理
- 请求响应正常

**涉及模块**: Protocol (Http2Handler)

**测试文件**: test_http2.cpp

---

### 4.12 用例 12: 连接超时检测

**用例 ID**: IT-012

**测试名称**: 连接超时检测

**优先级**: 中

**前置条件**: IT-002 执行成功

**测试步骤**:
1. 配置 Server 连接超时为 10 秒
2. 启动 Server
3. SimClient 建立连接但不发送数据
4. 等待 15 秒
5. 检查连接状态
6. 停止 Server

**预期结果**:
- 连接在约 10 秒后被 Server 关闭
- ConnectionManager 正确清理超时连接

**涉及模块**: Connection, ConnectionManager

**测试文件**: test_connection_timeout.cpp

---

### 4.13 用例 13: 统计信息收集

**用例 ID**: IT-013

**测试名称**: 统计信息收集

**优先级**: 低

**前置条件**: IT-004 执行成功

**测试步骤**:
1. 启动 Server
2. 10 个 SimClient 各发送 10 个请求
3. 查询 Server 统计信息
4. 停止 SimClient 和 Server

**预期结果**:
- 总连接数 = 10
- 总请求数 = 100
- RPS 统计正确
- 延迟统计正确

**涉及模块**: Server, Utils (Statistics)

**测试文件**: test_statistics.cpp

---

### 4.14 用例 14: TLS 1.2 握手

**用例 ID**: IT-014

**测试名称**: TLS 1.2 握手

**优先级**: 高

**前置条件**: IT-002 执行成功

**测试步骤**:
1. 配置 Server 支持 TLS 1.2
2. 启动 Server
3. SimClient 配置使用 TLS 1.2
4. SimClient 连接并发送请求
5. 验证握手成功
6. 停止 SimClient 和 Server

**预期结果**:
- TLS 1.2 握手成功
- 请求响应正常

**涉及模块**: Protocol (TlsHandler)

**测试文件**: test_tls_1_2.cpp

---

### 4.15 用例 15: TLS 1.3 握手

**用例 ID**: IT-015

**测试名称**: TLS 1.3 握手

**优先级**: 高

**前置条件**: IT-014 执行成功

**测试步骤**:
1. 配置 Server 支持 TLS 1.3
2. 启动 Server
3. SimClient 配置使用 TLS 1.3
4. SimClient 连接并发送请求
5. 验证握手成功
6. 停止 SimClient 和 Server

**预期结果**:
- TLS 1.3 握手成功
- 请求响应正常

**涉及模块**: Protocol (TlsHandler)

**测试文件**: test_tls_1_3.cpp

---

### 4.16 用例 16: TLS 证书验证 - 有效证书

**用例 ID**: IT-016

**测试名称**: TLS 证书验证 - 有效证书

**优先级**: 高

**前置条件**: IT-014 执行成功

**测试步骤**:
1. 配置 Server 使用有效证书
2. 配置 Server 开启客户端证书验证
3. 启动 Server
4. SimClient 配置使用有效客户端证书
5. SimClient 连接并发送请求
6. 停止 SimClient 和 Server

**预期结果**:
- 证书验证通过
- 连接成功
- 请求响应正常

**涉及模块**: Protocol (TlsHandler)

**测试文件**: test_tls_valid_cert.cpp

---

### 4.17 用例 17: TLS 证书验证 - 无效证书

**用例 ID**: IT-017

**测试名称**: TLS 证书验证 - 无效证书

**优先级**: 高

**前置条件**: IT-016 执行成功

**测试步骤**:
1. 配置 Server 使用有效证书
2. 配置 Server 开启客户端证书验证
3. 启动 Server
4. SimClient 配置使用无效/过期证书
5. SimClient 尝试连接
6. 观察连接结果
7. 停止 Server

**预期结果**:
- 证书验证失败
- 连接被 Server 拒绝

**涉及模块**: Protocol (TlsHandler)

**测试文件**: test_tls_invalid_cert.cpp

---

### 4.18 用例 18: TLS 证书验证 - 无证书

**用例 ID**: IT-018

**测试名称**: TLS 证书验证 - 无证书

**优先级**: 高

**前置条件**: IT-016 执行成功

**测试步骤**:
1. 配置 Server 使用有效证书
2. 配置 Server 开启客户端证书验证
3. 启动 Server
4. SimClient 不提供证书
5. SimClient 尝试连接
6. 观察连接结果
7. 停止 Server

**预期结果**:
- 证书验证失败
- 连接被 Server 拒绝

**涉及模块**: Protocol (TlsHandler)

**测试文件**: test_tls_no_cert.cpp

---

### 4.19 用例 19: TLS 密码套件 - AES-GCM

**用例 ID**: IT-019

**测试名称**: TLS 密码套件 - AES-GCM

**优先级**: 中

**前置条件**: IT-014 执行成功

**测试步骤**:
1. 配置 Server 使用 TLS_AES_256_GCM_SHA384
2. 启动 Server
3. SimClient 配置使用相同密码套件
4. SimClient 连接并发送请求
5. 停止 SimClient 和 Server

**预期结果**:
- 握手成功，协商使用 AES-GCM
- 请求响应正常

**涉及模块**: Protocol (TlsHandler)

**测试文件**: test_tls_cipher_aes_gcm.cpp

---

### 4.20 用例 20: TLS 密码套件 - ChaCha20-Poly1305

**用例 ID**: IT-020

**测试名称**: TLS 密码套件 - ChaCha20-Poly1305

**优先级**: 中

**前置条件**: IT-019 执行成功

**测试步骤**:
1. 配置 Server 使用 TLS_CHACHA20_POLY1305_SHA256
2. 启动 Server
3. SimClient 配置使用相同密码套件
4. SimClient 连接并发送请求
5. 停止 SimClient 和 Server

**预期结果**:
- 握手成功，协商使用 ChaCha20-Poly1305
- 请求响应正常

**涉及模块**: Protocol (TlsHandler)

**测试文件**: test_tls_cipher_chacha20.cpp

---

### 4.21 用例 21: TLS 国密 SM2 单证书

**用例 ID**: IT-021

**测试名称**: TLS 国密 SM2 单证书

**优先级**: 中

**前置条件**: IT-014 执行成功

**测试步骤**:
1. 配置 Server 使用国密 SM2 单证书
2. 启动 Server
3. SimClient 配置使用国密
4. SimClient 连接并发送请求
5. 停止 SimClient 和 Server

**预期结果**:
- 国密握手成功
- 请求响应正常

**涉及模块**: Protocol (TlsHandler)

**测试文件**: test_tls_gmssl_sm2_single.cpp

---

### 4.22 用例 22: TLS 国密 SM2 双证书

**用例 ID**: IT-022

**测试名称**: TLS 国密 SM2 双证书

**优先级**: 中

**前置条件**: IT-021 执行成功

**测试步骤**:
1. 配置 Server 使用国密 SM2 双证书（签名+加密）
2. 启动 Server
3. SimClient 配置使用国密
4. SimClient 连接并发送请求
5. 停止 SimClient 和 Server

**预期结果**:
- 国密握手成功
- 请求响应正常

**涉及模块**: Protocol (TlsHandler)

**测试文件**: test_tls_gmssl_sm2_dual.cpp

---

### 4.23 用例 23: DebugChain - 延迟响应 + 错误码叠加

**用例 ID**: IT-023

**测试名称**: DebugChain - 延迟响应 + 错误码叠加

**优先级**: 高

**前置条件**: IT-005 ~ IT-006 执行成功

**测试步骤**:
1. 配置 Server 开启 DebugChain，启用 DelayHandler 和 ErrorCodeHandler
2. 启动 Server
3. SimClient 添加 Header:
   - X-Debug-Response-Timeout-Ms: 2000
   - X-Debug-Force-Error-Code: 503
4. SimClient 发送请求
5. 测量响应时间，检查状态码
6. 停止 SimClient 和 Server

**预期结果**:
- 响应延迟约 2000ms
- 响应 HTTP 状态码为 503

**涉及模块**: DebugChain

**测试文件**: test_debug_delay_error_combined.cpp

---

### 4.24 用例 24: DebugChain - 延迟响应 + 强制断开叠加

**用例 ID**: IT-024

**测试名称**: DebugChain - 延迟响应 + 强制断开叠加

**优先级**: 高

**前置条件**: IT-005, IT-007 执行成功

**测试步骤**:
1. 配置 Server 开启 DebugChain，启用 DelayHandler 和 DisconnectHandler
2. 启动 Server
3. SimClient 添加 Header:
   - X-Debug-Response-Timeout-Ms: 1000
   - X-Debug-Force-Disconnect: true
4. SimClient 发送请求
5. 观察连接状态
6. 停止 SimClient 和 Server

**预期结果**:
- 响应延迟约 1000ms
- 响应后连接立即断开

**涉及模块**: DebugChain, Connection

**测试文件**: test_debug_delay_disconnect_combined.cpp

---

### 4.25 用例 25: DebugChain - 错误码 + 强制断开叠加

**用例 ID**: IT-025

**测试名称**: DebugChain - 错误码 + 强制断开叠加

**优先级**: 高

**前置条件**: IT-006, IT-007 执行成功

**测试步骤**:
1. 配置 Server 开启 DebugChain，启用 ErrorCodeHandler 和 DisconnectHandler
2. 启动 Server
3. SimClient 添加 Header:
   - X-Debug-Force-Error-Code: 404
   - X-Debug-Force-Disconnect: true
4. SimClient 发送请求
5. 检查状态码和连接状态
6. 停止 SimClient 和 Server

**预期结果**:
- 响应 HTTP 状态码为 404
- 响应后连接立即断开

**涉及模块**: DebugChain, Connection

**测试文件**: test_debug_error_disconnect_combined.cpp

---

### 4.26 用例 26: DebugChain - 延迟 + 错误码 + 强制断开三重叠加

**用例 ID**: IT-026

**测试名称**: DebugChain - 延迟 + 错误码 + 强制断开三重叠加

**优先级**: 高

**前置条件**: IT-023 ~ IT-025 执行成功

**测试步骤**:
1. 配置 Server 开启 DebugChain，启用所有 Handler
2. 启动 Server
3. SimClient 添加 Header:
   - X-Debug-Response-Timeout-Ms: 1500
   - X-Debug-Force-Error-Code: 500
   - X-Debug-Force-Disconnect: true
4. SimClient 发送请求
5. 验证所有效果
6. 停止 SimClient 和 Server

**预期结果**:
- 响应延迟约 1500ms
- 响应 HTTP 状态码为 500
- 响应后连接立即断开

**涉及模块**: DebugChain, Connection

**测试文件**: test_debug_triple_combined.cpp

---

### 4.27 用例 27: DebugChain - 日志记录调测点

**用例 ID**: IT-027

**测试名称**: DebugChain - 日志记录调测点

**优先级**: 中

**前置条件**: IT-005 执行成功

**测试步骤**:
1. 配置 Server 开启 DebugChain，启用 LogHandler
2. 配置 Server 日志级别为 DEBUG
3. 启动 Server
4. SimClient 发送请求
5. 检查日志输出
6. 停止 SimClient 和 Server

**预期结果**:
- 日志中记录了请求信息
- 日志中记录了响应信息

**涉及模块**: DebugChain, Utils (Logger)

**测试文件**: test_debug_logging.cpp

---

### 4.28 用例 28: Client 中途断开 - TLS 握手阶段

**用例 ID**: IT-028

**测试名称**: Client 中途断开 - TLS 握手阶段

**优先级**: 高

**前置条件**: IT-002 执行成功

**测试步骤**:
1. 启动 Server
2. SimClient 开始连接，在 TLS 握手过程中主动断开
3. 观察 Server 状态
4. 检查连接是否被清理
5. 停止 Server

**预期结果**:
- Server 检测到连接断开
- 连接被正确清理
- Server 继续运行，不崩溃

**涉及模块**: Connection, ConnectionManager

**测试文件**: test_client_disconnect_during_handshake.cpp

---

### 4.29 用例 29: Client 中途断开 - 接收请求阶段

**用例 ID**: IT-029

**测试名称**: Client 中途断开 - 接收请求阶段

**优先级**: 高

**前置条件**: IT-028 执行成功

**测试步骤**:
1. 启动 Server
2. SimClient 连接成功
3. SimClient 发送部分请求头后主动断开
4. 观察 Server 状态
5. 检查连接是否被清理
6. 停止 Server

**预期结果**:
- Server 检测到连接断开
- 连接被正确清理
- Server 继续运行，不崩溃

**涉及模块**: Connection, ConnectionManager, Protocol

**测试文件**: test_client_disconnect_during_recv.cpp

---

### 4.30 用例 30: Client 中途断开 - 回调处理阶段

**用例 ID**: IT-030

**测试名称**: Client 中途断开 - 回调处理阶段

**优先级**: 高

**前置条件**: IT-029 执行成功

**测试步骤**:
1. 配置回调处理耗时 2 秒
2. 启动 Server
3. SimClient 发送完整请求
4. 在回调处理过程中（约 1 秒时）SimClient 主动断开
5. 观察 Server 状态
6. 检查回调是否继续完成
7. 停止 Server

**预期结果**:
- Server 继续完成回调处理
- 回调完成后连接被正确清理
- Server 继续运行，不崩溃

**涉及模块**: Connection, ConnectionManager, MsgCenter, Callback

**测试文件**: test_client_disconnect_during_callback.cpp

---

### 4.31 用例 31: Client 中途断开 - 发送响应阶段

**用例 ID**: IT-031

**测试名称**: Client 中途断开 - 发送响应阶段

**优先级**: 高

**前置条件**: IT-030 执行成功

**测试步骤**:
1. 启动 Server
2. SimClient 发送请求
3. Server 开始发送响应，发送部分数据后 SimClient 主动断开
4. 观察 Server 状态
5. 检查连接是否被清理
6. 停止 Server

**预期结果**:
- Server 检测到连接断开
- 连接被正确清理
- Server 继续运行，不崩溃

**涉及模块**: Connection, ConnectionManager, Protocol

**测试文件**: test_client_disconnect_during_send.cpp

---

### 4.32 用例 32: 多个 Client 同时中途断开

**用例 ID**: IT-032

**测试名称**: 多个 Client 同时中途断开

**优先级**: 高

**前置条件**: IT-028 ~ IT-031 执行成功

**测试步骤**:
1. 启动 Server
2. 同时启动 20 个 SimClient
3. 每个 SimClient 在不同阶段断开（握手、接收、回调、发送）
4. 观察 Server 状态
5. 检查所有连接是否被清理
6. 停止 Server

**预期结果**:
- 所有连接被正确清理
- Server 继续运行，不崩溃
- 无资源泄漏

**涉及模块**: Connection, ConnectionManager

**测试文件**: test_multiple_clients_disconnect.cpp

---

### 4.33 用例 33: HTTP/1.1 GET 请求

**用例 ID**: IT-033

**测试名称**: HTTP/1.1 GET 请求

**优先级**: 高

**前置条件**: IT-002 执行成功

**测试步骤**:
1. 启动 Server
2. SimClient 发送 HTTP/1.1 GET 请求
3. 验证响应
4. 停止 SimClient 和 Server

**预期结果**:
- GET 请求正确解析
- 响应正常返回

**涉及模块**: Protocol (Http1Handler)

**测试文件**: test_http1_get.cpp

---

### 4.34 用例 34: HTTP/1.1 POST 请求（带 body）

**用例 ID**: IT-034

**测试名称**: HTTP/1.1 POST 请求（带 body）

**优先级**: 高

**前置条件**: IT-033 执行成功

**测试步骤**:
1. 启动 Server
2. SimClient 发送 HTTP/1.1 POST 请求，带 1KB body
3. 验证响应
4. 停止 SimClient 和 Server

**预期结果**:
- POST 请求正确解析
- Body 正确接收
- 响应正常返回

**涉及模块**: Protocol (Http1Handler)

**测试文件**: test_http1_post_with_body.cpp

---

### 4.35 用例 35: HTTP/1.1 分块传输编码

**用例 ID**: IT-035

**测试名称**: HTTP/1.1 分块传输编码

**优先级**: 中

**前置条件**: IT-034 执行成功

**测试步骤**:
1. 启动 Server
2. SimClient 发送 HTTP/1.1 请求，使用 Transfer-Encoding: chunked
3. 验证响应
4. 停止 SimClient 和 Server

**预期结果**:
- 分块编码正确解析
- 响应正常返回

**涉及模块**: Protocol (Http1Handler)

**测试文件**: test_http1_chunked.cpp

---

### 4.36 用例 36: HTTP/1.1 大量请求头

**用例 ID**: IT-036

**测试名称**: HTTP/1.1 大量请求头

**优先级**: 中

**前置条件**: IT-033 执行成功

**测试步骤**:
1. 启动 Server
2. SimClient 发送 HTTP/1.1 请求，带 50 个请求头
3. 验证响应
4. 停止 SimClient 和 Server

**预期结果**:
- 请求头正确解析
- 响应正常返回

**涉及模块**: Protocol (Http1Handler)

**测试文件**: test_http1_many_headers.cpp

---

### 4.37 用例 37: HTTP/2 基础请求

**用例 ID**: IT-037

**测试名称**: HTTP/2 基础请求

**优先级**: 高

**前置条件**: IT-011 执行成功

**测试步骤**:
1. 配置 Server 启用 HTTP/2
2. 启动 Server
3. SimClient 使用 HTTP/2 发送请求
4. 验证响应
5. 停止 SimClient 和 Server

**预期结果**:
- ALPN 协商成功选择 h2
- HTTP/2 帧正确处理
- 响应正常返回

**涉及模块**: Protocol (Http2Handler)

**测试文件**: test_http2_basic.cpp

---

### 4.38 用例 38: HTTP/2 多路复用 - 并发流

**用例 ID**: IT-038

**测试名称**: HTTP/2 多路复用 - 并发流

**优先级**: 高

**前置条件**: IT-037 执行成功

**测试步骤**:
1. 配置 Server 启用 HTTP/2
2. 启动 Server
3. SimClient 使用 HTTP/2，在同一连接上同时发送 10 个请求（不同 stream ID）
4. 验证所有响应
5. 停止 SimClient 和 Server

**预期结果**:
- 10 个流都正确处理
- 10 个响应都正确返回
- 连接保持

**涉及模块**: Protocol (Http2Handler)

**测试文件**: test_http2_concurrent_streams.cpp

---

### 4.39 用例 39: HTTP/2 服务端推送

**用例 ID**: IT-039

**测试名称**: HTTP/2 服务端推送

**优先级**: 中

**前置条件**: IT-037 执行成功

**测试步骤**:
1. 配置 Server 启用 HTTP/2 和服务端推送
2. 启动 Server
3. SimClient 发送请求，允许服务端推送
4. 验证主响应和推送资源
5. 停止 SimClient 和 Server

**预期结果**:
- 主响应正确返回
- 推送资源正确接收

**涉及模块**: Protocol (Http2Handler)

**测试文件**: test_http2_server_push.cpp

---

### 4.40 用例 40: HTTP/1.1 升级到 HTTP/2

**用例 ID**: IT-040

**测试名称**: HTTP/1.1 升级到 HTTP/2 (h2c)

**优先级**: 中

**前置条件**: IT-011, IT-033 执行成功

**测试步骤**:
1. 配置 Server 允许 h2c
2. 启动 Server
3. SimClient 发送 HTTP/1.1 Upgrade 请求
4. 验证协议升级
5. 停止 SimClient 和 Server

**预期结果**:
- 协议升级成功
- 使用 HTTP/2 继续通信

**涉及模块**: Protocol (Http2Handler)

**测试文件**: test_http2_upgrade.cpp

---

### 4.41 用例 41: 回调超时检测

**用例 ID**: IT-041

**测试名称**: 回调超时检测

**优先级**: 高

**前置条件**: IT-008 执行成功

**测试步骤**:
1. 配置回调超时为 5 秒
2. 注册一个耗时 10 秒的回调
3. 启动 Server
4. SimClient 发送请求
5. 等待 8 秒
6. 检查回调是否被标记为超时
7. 停止 SimClient 和 Server

**预期结果**:
- 回调超时被检测到
- 连接被正确处理

**涉及模块**: Connection, ConnectionManager, Callback

**测试文件**: test_callback_timeout.cpp

---

### 4.42 用例 42: 回调正常完成（刚好不超时）

**用例 ID**: IT-042

**测试名称**: 回调正常完成（刚好不超时）

**优先级**: 高

**前置条件**: IT-041 执行成功

**测试步骤**:
1. 配置回调超时为 5 秒
2. 注册一个耗时 4 秒的回调
3. 启动 Server
4. SimClient 发送请求
5. 等待回调完成
6. 验证响应
7. 停止 SimClient 和 Server

**预期结果**:
- 回调正常完成
- 响应正常返回

**涉及模块**: Connection, ConnectionManager, Callback

**测试文件**: test_callback_no_timeout.cpp

---

### 4.43 用例 43: 多端口 - 不同回调策略

**用例 ID**: IT-043

**测试名称**: 多端口 - 不同回调策略

**优先级**: 高

**前置条件**: IT-009 执行成功

**测试步骤**:
1. 配置 Server 监听 19443 和 19444 两个端口
2. 为 19443 注册回调策略 A
3. 为 19444 注册回调策略 B
4. 启动 Server
5. SimClient1 连接到 19443 发送请求
6. SimClient2 连接到 19444 发送请求
7. 验证两个响应来自不同回调
8. 停止 SimClient 和 Server

**预期结果**:
- 19443 的响应来自回调 A
- 19444 的响应来自回调 B

**涉及模块**: Server, Callback, ConnectionManager

**测试文件**: test_multi_port_different_callbacks.cpp

---

### 4.44 用例 44: Graceful Shutdown - 等待现有连接

**用例 ID**: IT-044

**测试名称**: Graceful Shutdown - 等待现有连接

**优先级**: 高

**前置条件**: IT-010 执行成功

**测试步骤**:
1. 启动 Server
2. 启动 5 个 SimClient，每个发送长耗时请求（耗时 3 秒）
3. 1 秒后调用 server.stop()
4. 等待 Server 完全停止
5. 检查所有请求是否完成
6. 检查 Server 状态

**预期结果**:
- Server 停止接受新连接
- 现有 5 个请求都处理完成
- Server 正常停止

**涉及模块**: Server, ConnectionManager

**测试文件**: test_graceful_shutdown_wait.cpp

---

### 4.45 用例 45: Graceful Shutdown - 超时强制关闭

**用例 ID**: IT-045

**测试名称**: Graceful Shutdown - 超时强制关闭

**优先级**: 高

**前置条件**: IT-044 执行成功

**测试步骤**:
1. 配置 Server Graceful Shutdown 超时为 5 秒
2. 启动 Server
3. 启动 5 个 SimClient，每个发送长耗时请求（耗时 10 秒）
4. 1 秒后调用 server.stop()
5. 等待 Server 停止
6. 检查 Server 是否在 5-6 秒内停止

**预期结果**:
- Server 在约 5-6 秒内停止
- 未完成的请求被强制终止
- 资源正确清理

**涉及模块**: Server, ConnectionManager

**测试文件**: test_graceful_shutdown_timeout.cpp

---

### 4.46 用例 46: 配置热重载（如果支持）

**用例 ID**: IT-046

**测试名称**: 配置热重载

**优先级**: 中

**前置条件**: IT-001 执行成功

**测试步骤**:
1. 创建初始配置 A
2. 启动 Server
3. 修改配置文件为配置 B
4. 触发配置热重载
5. 验证配置生效
6. 停止 Server

**预期结果**:
- 配置 B 生效
- Server 继续运行

**涉及模块**: Server, Config

**测试文件**: test_config_reload.cpp

---

### 4.47 用例 47: 日志级别动态调整

**用例 ID**: IT-047

**测试名称**: 日志级别动态调整

**优先级**: 低

**前置条件**: IT-001 执行成功

**测试步骤**:
1. 配置 Server 日志级别为 INFO
2. 启动 Server
3. 发送一些请求，观察日志
4. 动态调整日志级别为 DEBUG
5. 发送更多请求，观察日志
6. 停止 Server

**预期结果**:
- INFO 级别时只输出 INFO 及以上日志
- DEBUG 级别时输出 DEBUG 及以上日志

**涉及模块**: Utils (Logger)

**测试文件**: test_log_level_change.cpp

---

### 4.48 用例 48: 连接数限制

**用例 ID**: IT-048

**测试名称**: 连接数限制

**优先级**: 高

**前置条件**: IT-004 执行成功

**测试步骤**:
1. 配置 Server 最大连接数为 10
2. 启动 Server
3. 尝试启动 15 个 SimClient
4. 观察连接结果
5. 停止 Server

**预期结果**:
- 前 10 个连接成功
- 后 5 个连接被拒绝
- 连接数不超过 10

**涉及模块**: Server, ConnectionManager

**测试文件**: test_connection_limit.cpp

---

### 4.49 用例 49: 快速重启 - 端口复用

**用例 ID**: IT-049

**测试名称**: 快速重启 - 端口复用

**优先级**: 中

**前置条件**: IT-001 执行成功

**测试步骤**:
1. 启动 Server
2. 立即停止 Server
3. 立即再次启动 Server
4. 验证启动成功
5. 停止 Server

**预期结果**:
- 第二次启动成功
- 端口正确复用

**涉及模块**: Server

**测试文件**: test_fast_restart.cpp

---

### 4.50 用例 50: 异常恢复 - 配置文件损坏

**用例 ID**: IT-050

**测试名称**: 异常恢复 - 配置文件损坏

**优先级**: 中

**前置条件**: IT-001 执行成功

**测试步骤**:
1. 创建损坏的配置文件
2. 尝试使用损坏的配置启动 Server
3. 验证启动失败
4. 恢复正确的配置文件
5. 再次启动 Server
6. 验证启动成功
7. 停止 Server

**预期结果**:
- 损坏配置时启动失败
- 正确配置时启动成功
- Server 状态正确

**涉及模块**: Server, Config

**测试文件**: test_config_corruption_recovery.cpp

---

### 4.51 用例 51: TLS 重新协商

**用例 ID**: IT-051

**测试名称**: TLS 重新协商

**优先级**: 中

**前置条件**: IT-014 执行成功

**测试步骤**:
1. 配置 Server 支持 TLS 重新协商
2. 启动 Server
3. SimClient 连接并发送请求
4. SimClient 触发 TLS 重新协商
5. 验证重新协商成功
6. 继续发送请求
7. 停止 SimClient 和 Server

**预期结果**:
- TLS 重新协商成功
- 新参数生效
- 请求继续正常处理

**涉及模块**: Protocol (TlsHandler)

**测试文件**: test_tls_renegotiation.cpp

---

### 4.52 用例 52: TLS 证书链验证 - 完整链

**用例 ID**: IT-052

**测试名称**: TLS 证书链验证 - 完整链

**优先级**: 高

**前置条件**: IT-016 执行成功

**测试步骤**:
1. 配置 Server 使用完整证书链（Root CA → Intermediate CA → Server Cert）
2. 配置 Server 开启客户端证书验证
3. 启动 Server
4. SimClient 使用完整客户端证书链
5. SimClient 连接并发送请求
6. 停止 SimClient 和 Server

**预期结果**:
- 证书链验证通过
- 连接成功
- 请求响应正常

**涉及模块**: Protocol (TlsHandler)

**测试文件**: test_tls_cert_chain_complete.cpp

---

### 4.53 用例 53: TLS 证书链验证 - 缺失中间 CA

**用例 ID**: IT-053

**测试名称**: TLS 证书链验证 - 缺失中间 CA

**优先级**: 高

**前置条件**: IT-052 执行成功

**测试步骤**:
1. 配置 Server 使用完整证书链，但不提供中间 CA
2. 配置 Server 开启客户端证书验证
3. 启动 Server
4. SimClient 尝试连接，只提供叶子证书
5. 观察连接结果
6. 停止 Server

**预期结果**:
- 证书链验证失败（无法追溯到信任根）
- 连接被拒绝

**涉及模块**: Protocol (TlsHandler)

**测试文件**: test_tls_cert_chain_missing_intermediate.cpp

---

### 4.54 用例 54: TLS 证书链验证 - 自签名证书

**用例 ID**: IT-054

**测试名称**: TLS 证书链验证 - 自签名证书

**优先级**: 中

**前置条件**: IT-052 执行成功

**测试步骤**:
1. 配置 Server 使用自签名证书
2. 配置 Server 信任该自签名证书
3. 启动 Server
4. SimClient 连接并发送请求
5. 停止 SimClient 和 Server

**预期结果**:
- 自签名证书验证通过（因为已配置信任）
- 连接成功
- 请求响应正常

**涉及模块**: Protocol (TlsHandler)

**测试文件**: test_tls_self_signed_cert.cpp

---

### 4.55 用例 55: TLS 会话恢复 - Session ID

**用例 ID**: IT-055

**测试名称**: TLS 会话恢复 - Session ID

**优先级**: 中

**前置条件**: IT-014 执行成功

**测试步骤**:
1. 配置 Server 支持 Session ID 会话恢复
2. 启动 Server
3. SimClient 第一次连接，保存 Session ID
4. SimClient 断开连接
5. SimClient 第二次连接，使用保存的 Session ID
6. 验证会话恢复成功
7. 停止 SimClient 和 Server

**预期结果**:
- 第二次连接使用会话恢复
- 握手更快（省略完整握手）
- 连接成功

**涉及模块**: Protocol (TlsHandler)

**测试文件**: test_tls_session_resumption_id.cpp

---

### 4.56 用例 56: TLS 会话恢复 - Session Ticket

**用例 ID**: IT-056

**测试名称**: TLS 会话恢复 - Session Ticket

**优先级**: 中

**前置条件**: IT-055 执行成功

**测试步骤**:
1. 配置 Server 支持 Session Ticket 会话恢复
2. 启动 Server
3. SimClient 第一次连接，获取 Session Ticket
4. SimClient 断开连接
5. SimClient 第二次连接，使用 Session Ticket
6. 验证会话恢复成功
7. 停止 SimClient 和 Server

**预期结果**:
- 第二次连接使用会话恢复
- 握手更快
- 连接成功

**涉及模块**: Protocol (TlsHandler)

**测试文件**: test_tls_session_resumption_ticket.cpp

---

### 4.57 用例 57: TLS ALPN 协议协商 - 成功

**用例 ID**: IT-057

**测试名称**: TLS ALPN 协议协商 - 成功

**优先级**: 高

**前置条件**: IT-014 执行成功

**测试步骤**:
1. 配置 Server 支持 ALPN，提供 ["h2", "http/1.1"]
2. 启动 Server
3. SimClient 配置 ALPN，提供 ["h2", "http/1.1"]
4. SimClient 连接
5. 验证协商结果为 "h2"
6. 停止 SimClient 和 Server

**预期结果**:
- ALPN 协商成功，选择 "h2"
- 连接成功

**涉及模块**: Protocol (TlsHandler)

**测试文件**: test_tls_alpn_success.cpp

---

### 4.58 用例 58: TLS ALPN 协议协商 - 无交集

**用例 ID**: IT-058

**测试名称**: TLS ALPN 协议协商 - 无交集

**优先级**: 中

**前置条件**: IT-057 执行成功

**测试步骤**:
1. 配置 Server 支持 ALPN，提供 ["h2"]
2. 启动 Server
3. SimClient 配置 ALPN，提供 ["http/1.1"]
4. SimClient 尝试连接
5. 观察连接结果
6. 停止 Server

**预期结果**:
- ALPN 协商失败（无交集）
- 连接被关闭或继续使用默认协议

**涉及模块**: Protocol (TlsHandler)

**测试文件**: test_tls_alpn_no_overlap.cpp

---

### 4.59 用例 59: TLS SNI (Server Name Indication)

**用例 ID**: IT-059

**测试名称**: TLS SNI (Server Name Indication)

**优先级**: 中

**前置条件**: IT-014 执行成功

**测试步骤**:
1. 配置 Server 支持 SNI
2. 启动 Server
3. SimClient 连接时提供 SNI 主机名
4. 验证 Server 识别 SNI
5. 停止 SimClient 和 Server

**预期结果**:
- SNI 被正确识别
- 连接成功

**涉及模块**: Protocol (TlsHandler)

**测试文件**: test_tls_sni.cpp

---

### 4.60 用例 60: 连接空闲超时

**用例 ID**: IT-060

**测试名称**: 连接空闲超时

**优先级**: 高

**前置条件**: IT-012 执行成功

**测试步骤**:
1. 配置 Server 连接空闲超时为 30 秒
2. 启动 Server
3. SimClient 连接成功
4. SimClient 不发送任何数据
5. 等待 40 秒
6. 检查连接状态
7. 停止 Server

**预期结果**:
- 连接在约 30 秒后被 Server 关闭
- 连接被正确清理

**涉及模块**: Connection, ConnectionManager

**测试文件**: test_connection_idle_timeout.cpp

---

## 5. 性能测试用例

### 5.1 用例 14: 性能基准测试 - 延迟

**用例 ID**: PERF-001

**测试名称**: 性能基准测试 - 延迟

**优先级**: 高

**前置条件**: IT-002 执行成功

**测试步骤**:
1. 启动 Server
2. 1 个 SimClient 发送 1000 个请求（连续发送）
3. 记录每个请求的响应时间
4. 计算 P50、P95、P99 延迟
5. 停止 SimClient 和 Server

**预期结果**:
- P50 延迟 < 10ms
- P95 延迟 < 50ms
- P99 延迟 < 100ms

**涉及模块**: Server, Connection, Protocol, MsgCenter

**测试文件**: test_perf_latency.cpp

---

### 5.2 用例 15: 性能基准测试 - 吞吐量（RPS）

**用例 ID**: PERF-002

**测试名称**: 性能基准测试 - 吞吐量（RPS）

**优先级**: 高

**前置条件**: PERF-001 执行成功

**测试步骤**:
1. 启动 Server
2. 10 个并发 SimClient，每个发送 1000 个请求
3. 持续 30 秒
4. 统计总请求数和平均 RPS
5. 停止 SimClient 和 Server

**预期结果**:
- 总请求数 > 10000
- 平均 RPS > 1000

**涉及模块**: Server, ConnectionManager, MsgCenter

**测试文件**: test_perf_throughput.cpp

---

### 5.3 用例 16: 性能测试 - 大报文处理

**用例 ID**: PERF-003

**测试名称**: 性能测试 - 大报文处理

**优先级**: 中

**前置条件**: IT-002 执行成功

**测试步骤**:
1. 启动 Server
2. 1 个 SimClient 发送以下大小的请求：
   - 1KB
   - 64KB
   - 1MB
   - 10MB
3. 记录每个大小的处理时间
4. 停止 SimClient 和 Server

**预期结果**:
- 1KB: < 10ms
- 64KB: < 50ms
- 1MB: < 500ms
- 10MB: < 5000ms

**涉及模块**: Protocol, Connection (Buffer)

**测试文件**: test_perf_large_payload.cpp

---

## 6. 压力测试用例

### 6.1 用例 17: 压力测试 - 100 并发连接

**用例 ID**: STRESS-001

**测试名称**: 压力测试 - 100 并发连接

**优先级**: 高

**前置条件**: IT-004 执行成功

**测试步骤**:
1. 配置 Server 最大连接数为 1000
2. 启动 Server
3. 同时启动 100 个 SimClient（端口 20001-20100）
4. 每个 SimClient 发送 100 个请求
5. 等待所有请求完成
6. 检查连接状态和错误率
7. 停止 SimClient 和 Server

**预期结果**:
- 所有 100 个连接成功建立
- 所有 10000 个请求成功处理
- 错误率 < 0.1%
- Server 资源正常释放

**涉及模块**: Server, ConnectionManager, MsgCenter

**测试文件**: test_stress_100_connections.cpp

---

### 6.2 用例 18: 压力测试 - 1000 并发连接

**用例 ID**: STRESS-002

**测试名称**: 压力测试 - 1000 并发连接

**优先级**: 高

**前置条件**: STRESS-001 执行成功

**测试步骤**:
1. 配置 Server 最大连接数为 2000
2. 启动 Server
3. 同时启动 1000 个 SimClient（端口 20001-21000）
4. 每个 SimClient 发送 10 个请求
5. 等待所有请求完成
6. 检查连接状态和错误率
7. 停止 SimClient 和 Server

**预期结果**:
- 连接成功率 > 99%
- 请求成功率 > 99%
- Server 不崩溃
- Server 资源正常释放

**涉及模块**: Server, ConnectionManager, MsgCenter

**测试文件**: test_stress_1000_connections.cpp

---

### 6.3 用例 19: 压力测试 - 长连接持久化

**用例 ID**: STRESS-003

**测试名称**: 压力测试 - 长连接持久化

**优先级**: 中

**前置条件**: IT-003 执行成功

**测试步骤**:
1. 启动 Server
2. 启动 50 个 SimClient，保持连接 5 分钟
3. 每个 SimClient 每隔 1 秒发送 1 个请求
4. 持续 5 分钟
5. 检查连接是否保持
6. 停止 SimClient 和 Server

**预期结果**:
- 所有 50 个连接在 5 分钟内保持活跃
- 无异常断开
- 所有请求成功处理

**涉及模块**: Connection, ConnectionManager

**测试文件**: test_stress_long_connections.cpp

---

### 6.4 用例 20: 压力测试 - 频繁连接断开

**用例 ID**: STRESS-004

**测试名称**: 压力测试 - 频繁连接断开

**优先级**: 中

**前置条件**: IT-002 执行成功

**测试步骤**:
1. 启动 Server
2. 10 个 SimClient 循环执行：连接 → 发送 1 个请求 → 断开
3. 每个 SimClient 执行 100 次循环
4. 统计总连接数和成功率
5. 停止 SimClient 和 Server

**预期结果**:
- 总连接数 = 1000
- 连接成功率 = 100%
- 无资源泄漏（文件描述符、内存）

**涉及模块**: Server, ConnectionManager

**测试文件**: test_stress_connect_disconnect.cpp

---

### 6.5 用例 21: 边界条件 - 超大请求头

**用例 ID**: STRESS-005

**测试名称**: 边界条件 - 超大请求头

**优先级**: 高

**前置条件**: IT-036 执行成功

**测试步骤**:
1. 配置 Server 最大请求头大小为 8KB
2. 启动 Server
3. SimClient 发送请求，包含 10KB 的单个请求头
4. 观察 Server 响应
5. 停止 SimClient 和 Server

**预期结果**:
- Server 正确拒绝超大请求头
- 返回 431 Request Header Fields Too Large
- Server 继续运行，不崩溃

**涉及模块**: Protocol (Http1Handler)

**测试文件**: test_stress_boundary_large_header.cpp

---

### 6.6 用例 22: 边界条件 - 超大请求体

**用例 ID**: STRESS-006

**测试名称**: 边界条件 - 超大请求体

**优先级**: 高

**前置条件**: STRESS-005 执行成功

**测试步骤**:
1. 配置 Server 最大请求体大小为 10MB
2. 启动 Server
3. SimClient 发送请求，包含 50MB 的请求体
4. 观察 Server 响应
5. 停止 SimClient 和 Server

**预期结果**:
- Server 正确拒绝超大请求体
- 返回 413 Payload Too Large
- Server 继续运行，不崩溃
- 无内存泄漏

**涉及模块**: Protocol, Connection (Buffer)

**测试文件**: test_stress_boundary_large_body.cpp

---

### 6.7 用例 23: 边界条件 - 极短连接超时

**用例 ID**: STRESS-007

**测试名称**: 边界条件 - 极短连接超时

**优先级**: 中

**前置条件**: IT-012 执行成功

**测试步骤**:
1. 配置 Server 连接超时为 1 秒
2. 启动 Server
3. SimClient 连接成功
4. SimClient 等待 2 秒后发送请求
5. 观察连接状态
6. 停止 SimClient 和 Server

**预期结果**:
- 连接在 1 秒后被 Server 关闭
- 请求发送失败（连接已关闭）
- Server 继续运行

**涉及模块**: Connection, ConnectionManager

**测试文件**: test_stress_boundary_short_timeout.cpp

---

### 6.8 用例 24: 边界条件 - 零长度请求

**用例 ID**: STRESS-008

**测试名称**: 边界条件 - 零长度请求

**优先级**: 中

**前置条件**: IT-033 执行成功

**测试步骤**:
1. 启动 Server
2. SimClient 连接后立即发送空数据（0 字节）
3. 保持连接 5 秒
4. 观察 Server 行为
5. 停止 SimClient 和 Server

**预期结果**:
- Server 正确处理空数据
- 无崩溃
- 无资源泄漏
- 连接最终超时关闭

**涉及模块**: Connection, Protocol

**测试文件**: test_stress_boundary_zero_length_request.cpp

---

### 6.9 用例 25: 边界条件 - 慢速攻击（Slowloris）

**用例 ID**: STRESS-009

**测试名称**: 边界条件 - 慢速攻击（Slowloris）

**优先级**: 高

**前置条件**: IT-048 执行成功

**测试步骤**:
1. 配置 Server 最大连接数为 100，连接超时为 30 秒
2. 启动 Server
3. 启动 50 个 SimClient，每个连接后每秒发送 1 个字节的请求头
4. 持续 60 秒
5. 观察 Server 行为
6. 停止所有 SimClient 和 Server

**预期结果**:
- Server 正确超时清理慢速连接
- Server 资源不被耗尽
- Server 继续接受新的正常连接

**涉及模块**: Connection, ConnectionManager

**测试文件**: test_stress_boundary_slowloris.cpp

---

### 6.10 用例 26: 边界条件 - 请求头格式异常

**用例 ID**: STRESS-010

**测试名称**: 边界条件 - 请求头格式异常

**优先级**: 中

**前置条件**: IT-033 执行成功

**测试步骤**:
1. 启动 Server
2. SimClient 依次发送以下异常请求：
   - 只有冒号的 Header: ": value"
   - 没有冒号的 Header: "NoColon"
   - 空 Header 名称: ": value"
   - 超长 Header 名称（10000 字符）
   - 包含特殊字符的 Header 名称
3. 观察 Server 对每个请求的响应
4. 停止 SimClient 和 Server

**预期结果**:
- Server 正确拒绝格式异常的请求
- 返回 400 Bad Request
- Server 继续运行，不崩溃

**涉及模块**: Protocol (Http1Handler)

**测试文件**: test_stress_boundary_invalid_headers.cpp

---

### 6.11 用例 27: 边界条件 - 编码异常

**用例 ID**: STRESS-011

**测试名称**: 边界条件 - 编码异常

**优先级**: 中

**前置条件**: IT-034 执行成功

**测试步骤**:
1. 启动 Server
2. SimClient 发送以下异常请求：
   - 无效的 UTF-8 序列
   - 不完整的分块编码
   - Content-Length 与实际 body 长度不匹配
   - 负的 Content-Length
3. 观察 Server 响应
4. 停止 SimClient 和 Server

**预期结果**:
- Server 正确处理编码异常
- 返回适当的错误码（400 或 411）
- Server 继续运行，不崩溃

**涉及模块**: Protocol (Http1Handler)

**测试文件**: test_stress_boundary_encoding_errors.cpp

---

### 6.12 用例 28: 边界条件 - 端口耗尽

**用例 ID**: STRESS-012

**测试名称**: 边界条件 - 端口耗尽

**优先级**: 低

**前置条件**: STRESS-002 执行成功

**测试步骤**:
1. 配置 Server 最大连接数为 10000
2. 启动 Server
3. 尽可能多地启动 SimClient（直到端口耗尽）
4. 观察 Server 在端口耗尽时的行为
5. 停止所有 SimClient
6. 再次启动新的 SimClient 验证恢复能力

**预期结果**:
- Server 在系统资源耗尽前正确拒绝连接
- Server 不崩溃
- 资源释放后能正常接受新连接

**涉及模块**: Server, ConnectionManager

**测试文件**: test_stress_boundary_port_exhaustion.cpp

---

## 7. 测试工具

### 7.1 SimClient

**位置**: codes/core/test/sim_client/

**功能**:
- 绑定指定的本地 IP 和 Port
- HTTPS 支持（TLS 1.2/1.3）
- 支持自定义证书
- 按顺序独立发送多个请求
- 通过 HTTP Header 添加调试字段

**使用方式**:

```bash
cd codes/core/test
python start_test_client.py sim_client/example_config.json
```

**配置文件格式**: 见第 3.1 节

**调试 Header**:
- `X-Debug-Force-Error-Code`: 强制错误码
- `X-Debug-Response-Timeout-Ms`: 响应超时时间（毫秒）

---

## 8. 测试执行计划

### 8.1 执行顺序

1. 基础集成用例（IT-001 ~ IT-013）
2. TLS 相关用例（IT-014 ~ IT-060）
3. 性能测试用例（PERF-001 ~ PERF-003）
4. 压力测试用例（STRESS-001 ~ STRESS-012）

### 8.2 测试用例统计

| 类别 | 数量 | 说明 |
|------|------|------|
| 集成测试 (IT) | 60 | 模块集成、端到端、调测功能、TLS 等 |
| 性能测试 (PERF) | 3 | 延迟、吞吐量、大报文 |
| 压力测试 (STRESS) | 12 | 并发连接、边界条件、异常场景 |
| **总计** | **75** | |

### 8.3 执行命令

```bash
# 构建
cd build
cmake ../codes -DBUILD_TESTS=ON
make -j8

# 运行所有集成测试
./bin/core_integration_test

# 或运行单个测试
./bin/core_integration_test --gtest_filter=IT-001*

# 运行性能测试
./bin/core_integration_test --gtest_filter=PERF-*

# 运行压力测试
./bin/core_integration_test --gtest_filter=STRESS-*

# 运行边界条件压力测试
./bin/core_integration_test --gtest_filter=STRESS-00*
```

---

**文档结束**
