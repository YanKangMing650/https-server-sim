// =============================================================================
//  HTTPS Server Simulator - Callback Module
//  文件: callback.cpp
//  描述: CallbackStrategyManager实现和C接口Wrapper
//  版权: Copyright (c) 2026
// =============================================================================
#include "callback/callback.h"
#include <cstring>

namespace https_server_sim {

// ==================== CallbackStrategyManager实现 ====================

CallbackStrategyManager::CallbackStrategyManager() = default;

CallbackStrategyManager::~CallbackStrategyManager() = default;

int CallbackStrategyManager::register_callback(const CallbackStrategy* strategy) {
    if (!validate_strategy(strategy)) {
        return CALLBACK_ERR_INVALID_PARAM;
    }

    std::lock_guard<std::mutex> lock(callback_registry_mutex_);

    auto it = callback_map_.find(strategy->port);
    if (it != callback_map_.end()) {
        return CALLBACK_ERR_PORT_EXISTS;
    }

    // 创建shared_ptr管理的CallbackStrategy副本
    auto strategy_ptr = std::make_shared<CallbackStrategy>(*strategy);
    callback_map_.emplace(strategy->port, strategy_ptr);
    return CALLBACK_SUCCESS;
}

std::shared_ptr<const CallbackStrategy> CallbackStrategyManager::get_callback(uint16_t port) const {
    std::lock_guard<std::mutex> lock(callback_registry_mutex_);

    auto it = callback_map_.find(port);
    if (it != callback_map_.end()) {
        return it->second;
    }
    return nullptr;
}

int CallbackStrategyManager::deregister_callback(uint16_t port) {
    std::lock_guard<std::mutex> lock(callback_registry_mutex_);

    auto it = callback_map_.find(port);
    if (it == callback_map_.end()) {
        return CALLBACK_ERR_PORT_NOT_FOUND;
    }
    callback_map_.erase(it);
    return CALLBACK_SUCCESS;
}

void CallbackStrategyManager::clear() {
    std::lock_guard<std::mutex> lock(callback_registry_mutex_);
    callback_map_.clear();
}

size_t CallbackStrategyManager::get_callback_count() const {
    std::lock_guard<std::mutex> lock(callback_registry_mutex_);
    return callback_map_.size();
}

int CallbackStrategyManager::invoke_parse_callback(uint16_t port,
                                                    const ClientContext* ctx,
                                                    const uint8_t* in,
                                                    uint32_t inLen,
                                                    uint32_t* out_result) {
    if (!validate_parse_invoke_params(ctx, in, inLen, out_result)) {
        return CALLBACK_ERR_INVALID_PARAM;
    }

    std::shared_ptr<const CallbackStrategy> strategy;

    {
        std::lock_guard<std::mutex> lock(callback_registry_mutex_);

        auto it = callback_map_.find(port);
        if (it == callback_map_.end()) {
            return CALLBACK_ERR_STRATEGY_NOT_FOUND;
        }

        // 拷贝shared_ptr，延长策略生命周期，锁外可安全使用
        strategy = it->second;
    }

    try {
        uint32_t result = strategy->parse(ctx, in, inLen);
        *out_result = result;
        return CALLBACK_SUCCESS;
    } catch (...) {
        // 捕获用户回调抛出的所有异常，防止程序崩溃
        *out_result = 0;
        return CALLBACK_ERR_CALLBACK_EXCEPTION;
    }
}

int CallbackStrategyManager::invoke_reply_callback(uint16_t port,
                                                    const ClientContext* ctx,
                                                    uint8_t* out,
                                                    uint32_t* outLen,
                                                    uint32_t* out_result) {
    if (!validate_reply_invoke_params(ctx, out, outLen, out_result)) {
        return CALLBACK_ERR_INVALID_PARAM;
    }

    std::shared_ptr<const CallbackStrategy> strategy;

    {
        std::lock_guard<std::mutex> lock(callback_registry_mutex_);

        auto it = callback_map_.find(port);
        if (it == callback_map_.end()) {
            return CALLBACK_ERR_STRATEGY_NOT_FOUND;
        }

        // 拷贝shared_ptr，延长策略生命周期，锁外可安全使用
        strategy = it->second;
    }

    try {
        uint32_t result = strategy->reply(ctx, out, outLen);
        *out_result = result;
        return CALLBACK_SUCCESS;
    } catch (...) {
        // 捕获用户回调抛出的所有异常，防止程序崩溃
        *out_result = 0;
        *outLen = 0;
        return CALLBACK_ERR_CALLBACK_EXCEPTION;
    }
}

bool CallbackStrategyManager::validate_strategy(const CallbackStrategy* strategy) const {
    // Early return模式简化逻辑
    if (strategy == nullptr) {
        return false;
    }
    if (strategy->name == nullptr) {
        return false;
    }
    // 校验策略名称长度 - 使用strnlen避免手写循环
    // strnlen同时处理空字符串和长度限制两种情况
    size_t name_len = strnlen(strategy->name, CALLBACK_MAX_STRATEGY_NAME_LENGTH);
    if (name_len == 0 || name_len >= CALLBACK_MAX_STRATEGY_NAME_LENGTH) {
        return false;
    }
    if (strategy->port == 0) {
        return false;
    }
    if (strategy->parse == nullptr || strategy->reply == nullptr) {
        return false;
    }
    return true;
}

bool CallbackStrategyManager::validate_parse_invoke_params(const ClientContext* ctx,
                                                          const uint8_t* in,
                                                          uint32_t inLen,
                                                          uint32_t* out_result) const {
    if (ctx == nullptr) {
        return false;
    }
    if (inLen > 0 && in == nullptr) {
        return false;
    }
    if (out_result == nullptr) {
        return false;
    }
    return true;
}

bool CallbackStrategyManager::validate_reply_invoke_params(const ClientContext* ctx,
                                                          uint8_t* out,
                                                          uint32_t* outLen,
                                                          uint32_t* out_result) const {
    if (ctx == nullptr) {
        return false;
    }
    if (out == nullptr) {
        return false;
    }
    if (outLen == nullptr) {
        return false;
    }
    if (out_result == nullptr) {
        return false;
    }
    return true;
}

} // namespace https_server_sim

// ==================== C接口Wrapper实现 ====================

CallbackRegistry* callback_registry_create(void) {
    return reinterpret_cast<CallbackRegistry*>(new https_server_sim::CallbackStrategyManager());
}

void callback_registry_destroy(CallbackRegistry* registry) {
    if (registry != nullptr) {
        delete reinterpret_cast<https_server_sim::CallbackStrategyManager*>(registry);
    }
}

int callback_registry_register_strategy(CallbackRegistry* registry,
                                        const CallbackStrategy* strategy) {
    if (registry == nullptr) {
        return CALLBACK_ERR_INVALID_PARAM;
    }
    auto* manager = reinterpret_cast<https_server_sim::CallbackStrategyManager*>(registry);
    return manager->register_callback(strategy);
}

int callback_registry_get_strategy_copy(CallbackRegistry* registry,
                                         uint16_t port,
                                         CallbackStrategy* out_strategy) {
    if (registry == nullptr || out_strategy == nullptr) {
        return CALLBACK_ERR_INVALID_PARAM;
    }
    auto* manager = reinterpret_cast<https_server_sim::CallbackStrategyManager*>(registry);
    auto strategy_ptr = manager->get_callback(port);
    if (!strategy_ptr) {
        return CALLBACK_ERR_STRATEGY_NOT_FOUND;
    }
    // 安全拷贝策略数据
    *out_strategy = *strategy_ptr;
    return CALLBACK_SUCCESS;
}

int callback_registry_deregister_strategy(CallbackRegistry* registry,
                                           uint16_t port) {
    if (registry == nullptr) {
        return CALLBACK_ERR_INVALID_PARAM;
    }
    auto* manager = reinterpret_cast<https_server_sim::CallbackStrategyManager*>(registry);
    return manager->deregister_callback(port);
}

const CallbackStrategy* callback_registry_get_strategy(CallbackRegistry* registry,
                                                        uint16_t port) {
    // 注意：C接口仍然返回裸指针，存在安全隐患
    // 建议C用户使用 callback_registry_get_strategy_copy() 代替
    if (registry == nullptr) {
        return nullptr;
    }
    auto* manager = reinterpret_cast<https_server_sim::CallbackStrategyManager*>(registry);
    auto strategy_ptr = manager->get_callback(port);
    if (strategy_ptr) {
        return strategy_ptr.get();
    }
    return nullptr;
}

int callback_registry_invoke_parse_callback(CallbackRegistry* registry,
                                            uint16_t port,
                                            const ClientContext* ctx,
                                            const uint8_t* in,
                                            uint32_t inLen,
                                            uint32_t* out_result) {
    if (registry == nullptr) {
        return CALLBACK_ERR_INVALID_PARAM;
    }
    auto* manager = reinterpret_cast<https_server_sim::CallbackStrategyManager*>(registry);
    return manager->invoke_parse_callback(port, ctx, in, inLen, out_result);
}

int callback_registry_invoke_reply_callback(CallbackRegistry* registry,
                                            uint16_t port,
                                            const ClientContext* ctx,
                                            uint8_t* out,
                                            uint32_t* outLen,
                                            uint32_t* out_result) {
    if (registry == nullptr) {
        return CALLBACK_ERR_INVALID_PARAM;
    }
    auto* manager = reinterpret_cast<https_server_sim::CallbackStrategyManager*>(registry);
    return manager->invoke_reply_callback(port, ctx, out, outLen, out_result);
}

// 文件结束
