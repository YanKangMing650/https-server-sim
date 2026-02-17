// =============================================================================
//  HTTPS Server Simulator - Callback Module
//  文件: callback.h
//  描述: Callback模块C接口定义头文件
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include <stdint.h>
#include "callback/client_context.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==================== 错误码定义 ====================
// C++代码使用constexpr，C代码使用#define保持兼容性
#ifdef __cplusplus
constexpr int CALLBACK_SUCCESS              = 0;   // 操作成功
constexpr int CALLBACK_ERR_PORT_EXISTS     = -1;  // 端口已存在（注册时）
constexpr int CALLBACK_ERR_PORT_NOT_FOUND  = -2;  // 端口未找到（注销时）
constexpr int CALLBACK_ERR_STRATEGY_NOT_FOUND = -3; // 回调策略未找到（调用时）
constexpr int CALLBACK_ERR_INVALID_PARAM   = -4;  // 参数无效（空指针、空字符串等）
#else
#define CALLBACK_SUCCESS              0   // 操作成功
#define CALLBACK_ERR_PORT_EXISTS     -1   // 端口已存在（注册时）
#define CALLBACK_ERR_PORT_NOT_FOUND  -2   // 端口未找到（注销时）
#define CALLBACK_ERR_STRATEGY_NOT_FOUND -3 // 回调策略未找到（调用时）
#define CALLBACK_ERR_INVALID_PARAM   -4   // 参数无效（空指针、空字符串等）
#endif

// 回调函数类型定义（函数指针，不使用动态库）
typedef uint32_t (*AsyncParseContentFunc)(const ClientContext* ctx,
                                           const uint8_t* in,
                                           uint32_t inLen);

typedef uint32_t (*AsyncReplyContentFunc)(const ClientContext* ctx,
                                           uint8_t* out,
                                           uint32_t* outLen);

// 回调策略结构
typedef struct {
    const char* name;              // 策略名称（不可为NULL）
    uint16_t port;                 // 监听端口（1-65535）
    AsyncParseContentFunc parse;   // 解析回调函数指针（不可为NULL）
    AsyncReplyContentFunc reply;   // 响应回调函数指针（不可为NULL）
} CallbackStrategy;

// CallbackStrategyManager前置声明（C接口仍使用CallbackRegistry名称保持兼容性）
typedef struct CallbackStrategyManager CallbackRegistry;

// ==================== 对外C接口（与架构文档一致）====================

/**
 * @brief 创建回调注册表
 * @return 回调注册表指针，失败返回NULL
 */
CallbackRegistry* callback_registry_create(void);

/**
 * @brief 销毁回调注册表
 * @param registry 回调注册表指针，可为NULL
 */
void callback_registry_destroy(CallbackRegistry* registry);

/**
 * @brief 注册回调策略
 * @param registry 回调注册表指针，不可为NULL
 * @param strategy 回调策略指针，不可为NULL
 * @return CALLBACK_SUCCESS 注册成功
 *         CALLBACK_ERR_INVALID_PARAM 参数无效
 *         CALLBACK_ERR_PORT_EXISTS 端口已存在
 */
int callback_registry_register_strategy(CallbackRegistry* registry,
                                        const CallbackStrategy* strategy);

/**
 * @brief 获取指定端口的回调策略
 * @param registry 回调注册表指针，不可为NULL
 * @param port 端口号
 * @return 回调策略指针，未找到或参数无效返回NULL
 * @warning 返回的指针仅在以下条件下有效：
 *   1. 无其他线程修改registry（调用deregister或destroy）
 *   2. 该指针不应被长期持有，应短期使用
 *   3. registry销毁后，该指针立即失效
 */
const CallbackStrategy* callback_registry_get_strategy(CallbackRegistry* registry,
                                                        uint16_t port);

#ifdef __cplusplus
}
#endif

// 文件结束
