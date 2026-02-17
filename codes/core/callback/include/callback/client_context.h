// =============================================================================
//  HTTPS Server Simulator - Callback Module
//  文件: client_context.h
//  描述: 客户端上下文结构体定义
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ClientContext结构体定义
typedef struct {
    uint64_t connection_id;      // 连接唯一标识
    const char* client_ip;        // 客户端IP地址（字符串，可为NULL）
    uint16_t client_port;         // 客户端端口号
    uint16_t server_port;         // 服务器监听端口号
    const char* token;            // 调测token（可为NULL，生命周期由Connection模块管理）
} ClientContext;

#ifdef __cplusplus
}
#endif

// 文件结束
