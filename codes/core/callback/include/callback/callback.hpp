// =============================================================================
//  HTTPS Server Simulator - Callback Module
//  文件: callback.hpp
//  描述: CallbackStrategyManager C++类定义
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include "callback/callback.h"
#include <unordered_map>
#include <mutex>

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
     * @return 回调策略指针，未找到返回nullptr
     * @warning 返回的指针仅在以下条件下有效：
     *   1. 无其他线程修改registry（调用deregister_callback或clear）
     *   2. 建议优先使用invoke_*_callback方法而非直接操作返回的指针
     *   3. 该指针不应被长期持有，应在锁保护下短期使用
     */
    const CallbackStrategy* get_callback(uint16_t port) const;

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

    std::unordered_map<uint16_t, CallbackStrategy> callback_map_;
    mutable std::mutex callback_registry_mutex_;
};

} // namespace https_server_sim

// 文件结束
