// =============================================================================
//  HTTPS Server Simulator - Callback Module
//  文件: callback.h
//  描述: Callback模块C接口和C++类定义头文件
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include <stdint.h>
#include "callback/client_context.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==================== 常量定义 ====================
// 策略名称最大长度
#define CALLBACK_MAX_STRATEGY_NAME_LENGTH 1024

// ==================== 错误码定义 ====================
// 统一使用宏定义，C和C++都可以使用，避免维护两套常量
#define CALLBACK_SUCCESS              0   // 操作成功
#define CALLBACK_ERR_PORT_EXISTS     -1   // 端口已存在（注册时）
#define CALLBACK_ERR_PORT_NOT_FOUND  -2   // 端口未找到（注销时）
#define CALLBACK_ERR_STRATEGY_NOT_FOUND -3 // 回调策略未找到（调用时）
#define CALLBACK_ERR_INVALID_PARAM   -4   // 参数无效（空指针、空字符串等）
#define CALLBACK_ERR_CALLBACK_EXCEPTION -5 // 回调函数抛出异常

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
 * @brief 获取指定端口的回调策略（拷贝版本，线程安全）
 * @param registry 回调注册表指针，不可为NULL
 * @param port 端口号
 * @param out_strategy 输出参数，用于存储拷贝的策略数据，不可为NULL
 * @return CALLBACK_SUCCESS 获取成功
 *         CALLBACK_ERR_INVALID_PARAM 参数无效
 *         CALLBACK_ERR_STRATEGY_NOT_FOUND 策略未找到
 * @note 该函数会拷贝策略数据到out_strategy，返回后可安全持有
 */
int callback_registry_get_strategy_copy(CallbackRegistry* registry,
                                         uint16_t port,
                                         CallbackStrategy* out_strategy);

/**
 * @brief 注销指定端口的回调策略
 * @param registry 回调注册表指针，不可为NULL
 * @param port 端口号
 * @return CALLBACK_SUCCESS 注销成功
 *         CALLBACK_ERR_INVALID_PARAM 参数无效
 *         CALLBACK_ERR_PORT_NOT_FOUND 端口未找到
 */
int callback_registry_deregister_strategy(CallbackRegistry* registry,
                                           uint16_t port);

/**
 * @brief 获取指定端口的回调策略（旧版本，不推荐使用）
 * @param registry 回调注册表指针，不可为NULL
 * @param port 端口号
 * @return 回调策略指针，未找到或参数无效返回NULL
 * @deprecated 使用 callback_registry_get_strategy_copy() 代替
 * @warning 严重线程安全警告：
 *   1. 返回的裸指针在多线程环境下极易出现悬空指针问题
 *   2. 如果其他线程调用deregister_strategy或destroy，指针立即失效
 *   3. 强烈建议使用callback_registry_get_strategy_copy()获得安全拷贝
 *   4. 仅在单线程环境且确知无并发修改时才可谨慎使用
 */
const CallbackStrategy* callback_registry_get_strategy(CallbackRegistry* registry,
                                                        uint16_t port);

/**
 * @brief 调用指定端口的解析回调
 * @param registry 回调注册表指针，不可为NULL
 * @param port 端口号
 * @param ctx 客户端上下文指针，不可为NULL
 * @param in 输入数据指针，inLen>0时不可为NULL
 * @param inLen 输入数据长度
 * @param out_result 输出结果指针，不可为NULL，用于存储回调返回值
 * @return CALLBACK_SUCCESS 调用成功
 *         CALLBACK_ERR_INVALID_PARAM 参数无效
 *         CALLBACK_ERR_STRATEGY_NOT_FOUND 端口未注册
 */
int callback_registry_invoke_parse_callback(CallbackRegistry* registry,
                                            uint16_t port,
                                            const ClientContext* ctx,
                                            const uint8_t* in,
                                            uint32_t inLen,
                                            uint32_t* out_result);

/**
 * @brief 调用指定端口的响应回调
 * @param registry 回调注册表指针，不可为NULL
 * @param port 端口号
 * @param ctx 客户端上下文指针，不可为NULL
 * @param out 输出缓冲区指针，不可为NULL
 * @param outLen 输入时表示缓冲区大小，输出时表示实际写入长度
 * @param out_result 输出结果指针，不可为NULL，用于存储回调返回值
 * @return CALLBACK_SUCCESS 调用成功
 *         CALLBACK_ERR_INVALID_PARAM 参数无效
 *         CALLBACK_ERR_STRATEGY_NOT_FOUND 端口未注册
 */
int callback_registry_invoke_reply_callback(CallbackRegistry* registry,
                                            uint16_t port,
                                            const ClientContext* ctx,
                                            uint8_t* out,
                                            uint32_t* outLen,
                                            uint32_t* out_result);

#ifdef __cplusplus
}

// ==================== C++类定义 ====================

#include <unordered_map>
#include <mutex>
#include <memory>

namespace https_server_sim {

class CallbackStrategyManager {
public:
    /**
     * @brief 构造函数
     */
    CallbackStrategyManager();

    /**
     * @brief 析构函数
     */
    ~CallbackStrategyManager();

    // 禁止拷贝
    CallbackStrategyManager(const CallbackStrategyManager&) = delete;
    CallbackStrategyManager& operator=(const CallbackStrategyManager&) = delete;

    // ==================== 对外C接口对应方法 ====================

    /**
     * @brief 注册回调策略
     * @param strategy 回调策略指针，不可为nullptr
     * @return CALLBACK_SUCCESS 注册成功
     *         CALLBACK_ERR_INVALID_PARAM 参数无效
     *         CALLBACK_ERR_PORT_EXISTS 端口已存在
     */
    int register_callback(const CallbackStrategy* strategy);

    /**
     * @brief 获取指定端口的回调策略
     * @param port 端口号
     * @return 回调策略的shared_ptr，未找到返回空shared_ptr
     * @note 返回的shared_ptr可安全持有，即使策略被注销也不会失效
     */
    std::shared_ptr<const CallbackStrategy> get_callback(uint16_t port) const;

    // ==================== 回调调用方法 ====================

    /**
     * @brief 调用指定端口的解析回调
     * @param port 端口号
     * @param ctx 客户端上下文指针，不可为nullptr
     * @param in 输入数据指针，inLen>0时不可为nullptr
     * @param inLen 输入数据长度
     * @param out_result 输出结果指针，不可为nullptr，用于存储回调返回值
     * @return CALLBACK_SUCCESS 调用成功
     *         CALLBACK_ERR_INVALID_PARAM 参数无效
     *         CALLBACK_ERR_STRATEGY_NOT_FOUND 端口未注册
     */
    int invoke_parse_callback(uint16_t port,
                              const ClientContext* ctx,
                              const uint8_t* in,
                              uint32_t inLen,
                              uint32_t* out_result);

    /**
     * @brief 调用指定端口的响应回调
     * @param port 端口号
     * @param ctx 客户端上下文指针，不可为nullptr
     * @param out 输出缓冲区指针，不可为nullptr
     * @param outLen 输入时表示缓冲区大小，输出时表示实际写入长度
     * @param out_result 输出结果指针，不可为nullptr，用于存储回调返回值
     * @return CALLBACK_SUCCESS 调用成功
     *         CALLBACK_ERR_INVALID_PARAM 参数无效
     *         CALLBACK_ERR_STRATEGY_NOT_FOUND 端口未注册
     */
    int invoke_reply_callback(uint16_t port,
                              const ClientContext* ctx,
                              uint8_t* out,
                              uint32_t* outLen,
                              uint32_t* out_result);

    // ==================== 内部C++扩展方法 ====================

    /**
     * @brief 注销指定端口的回调策略
     * @param port 端口号
     * @return CALLBACK_SUCCESS 注销成功
     *         CALLBACK_ERR_PORT_NOT_FOUND 端口未找到
     */
    int deregister_callback(uint16_t port);

    /**
     * @brief 清空所有回调策略
     */
    void clear();

    /**
     * @brief 获取已注册的回调策略数量
     * @return 已注册的策略数量
     */
    size_t get_callback_count() const;

private:
    /**
     * @brief 校验回调策略的有效性
     * @param strategy 回调策略指针
     * @return true 策略有效，false 策略无效
     */
    bool validate_strategy(const CallbackStrategy* strategy) const;

    /**
     * @brief 校验invoke_parse_callback的参数
     * @param ctx 客户端上下文指针
     * @param in 输入数据指针
     * @param inLen 输入数据长度
     * @param out_result 输出结果指针
     * @return true 参数有效，false 参数无效
     */
    bool validate_parse_invoke_params(const ClientContext* ctx,
                                      const uint8_t* in,
                                      uint32_t inLen,
                                      uint32_t* out_result) const;

    /**
     * @brief 校验invoke_reply_callback的参数
     * @param ctx 客户端上下文指针
     * @param out 输出缓冲区指针
     * @param outLen 输出长度指针
     * @param out_result 输出结果指针
     * @return true 参数有效，false 参数无效
     */
    bool validate_reply_invoke_params(const ClientContext* ctx,
                                      uint8_t* out,
                                      uint32_t* outLen,
                                      uint32_t* out_result) const;

    std::unordered_map<uint16_t, std::shared_ptr<CallbackStrategy>> callback_map_;
    mutable std::mutex callback_registry_mutex_;
};

} // namespace https_server_sim

#endif

// 文件结束
