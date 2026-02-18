#pragma once

#include <cstdint>
#include "debug_chain/debug_config.hpp"
#include "debug_chain/debug_context.hpp"
#include "callback/client_context.h"

#ifdef __cplusplus
extern "C" {
#endif

// 前置声明
struct DebugHandler;

// 处理请求函数指针类型
typedef int (*DebugHandlerHandleRequestFunc)(struct DebugHandler* self,
                                              const ClientContext* ctx,
                                              const DebugConfig* config,
                                              DebugContext* debug_ctx);

// 处理响应函数指针类型
typedef int (*DebugHandlerHandleResponseFunc)(struct DebugHandler* self,
                                               const ClientContext* ctx,
                                               const DebugConfig* config,
                                               DebugContext* debug_ctx);

// 销毁函数指针类型
typedef void (*DebugHandlerDestroyFunc)(struct DebugHandler* self);

// DebugHandler结构体 - 调测点处理器C风格结构（全局命名空间，extern "C"内）
struct DebugHandler {
    const char* name;                                    // 调测点名称
    int priority;                                         // 优先级（越小越先执行）
    void* user_data;                                      // 用户数据
    DebugHandlerHandleRequestFunc handle_request;         // 处理请求函数指针
    DebugHandlerHandleResponseFunc handle_response;       // 处理响应函数指针
    DebugHandlerDestroyFunc destroy;                      // 销毁函数指针
};

#ifdef __cplusplus
}
#endif

// ========== C++命名空间内的创建函数声明 ==========
#ifdef __cplusplus
namespace https_server_sim {
namespace debug_chain {

// 创建DelayHandler - 延迟响应调测点
DebugHandler* create_delay_handler();

// 创建DisconnectHandler - 强制断开调测点
DebugHandler* create_disconnect_handler();

// 创建LogHandler - 日志记录调测点
DebugHandler* create_log_handler();

// 创建ErrorCodeHandler - 错误码调测点
DebugHandler* create_errorcode_handler();

} // namespace debug_chain
} // namespace https_server_sim
#endif

// 模块：DebugChain - 调测点处理器接口定义
// 用途：定义C风格的DebugHandler结构和创建函数，供C/C++代码混合使用
