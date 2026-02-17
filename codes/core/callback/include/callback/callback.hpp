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
    CallbackStrategyManager();
    ~CallbackStrategyManager();

    // 禁止拷贝
    CallbackStrategyManager(const CallbackStrategyManager&) = delete;
    CallbackStrategyManager& operator=(const CallbackStrategyManager&) = delete;

    // ==================== 对外C接口对应方法 ====================
    int register_callback(const CallbackStrategy* strategy);
    const CallbackStrategy* get_callback(uint16_t port) const;

    // ==================== 回调调用方法 ====================
    int invoke_parse_callback(uint16_t port,
                              const ClientContext* ctx,
                              const uint8_t* in,
                              uint32_t inLen,
                              uint32_t* out_result);

    int invoke_reply_callback(uint16_t port,
                              const ClientContext* ctx,
                              uint8_t* out,
                              uint32_t* outLen,
                              uint32_t* out_result);

    // ==================== 内部C++扩展方法 ====================
    int deregister_callback(uint16_t port);
    void clear();
    size_t get_callback_count() const;

private:
    bool validate_strategy(const CallbackStrategy* strategy) const;

    std::unordered_map<uint16_t, CallbackStrategy> callback_map_;
    mutable std::mutex callback_registry_mutex_;
};

} // namespace https_server_sim

// 文件结束
