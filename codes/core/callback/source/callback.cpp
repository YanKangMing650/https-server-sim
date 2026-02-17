// =============================================================================
//  HTTPS Server Simulator - Callback Module
//  文件: callback.cpp
//  描述: CallbackStrategyManager实现和C接口Wrapper
//  版权: Copyright (c) 2026
// =============================================================================
#include "callback/callback.hpp"

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

    callback_map_.emplace(strategy->port, *strategy);
    return CALLBACK_SUCCESS;
}

const CallbackStrategy* CallbackStrategyManager::get_callback(uint16_t port) const {
    std::lock_guard<std::mutex> lock(callback_registry_mutex_);

    auto it = callback_map_.find(port);
    if (it != callback_map_.end()) {
        return &(it->second);
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

    AsyncParseContentFunc parse_func = nullptr;

    {
        std::lock_guard<std::mutex> lock(callback_registry_mutex_);

        auto it = callback_map_.find(port);
        if (it == callback_map_.end()) {
            return CALLBACK_ERR_STRATEGY_NOT_FOUND;
        }

        parse_func = it->second.parse;
    }

    uint32_t result = parse_func(ctx, in, inLen);
    *out_result = result;
    return CALLBACK_SUCCESS;
}

int CallbackStrategyManager::invoke_reply_callback(uint16_t port,
                                                    const ClientContext* ctx,
                                                    uint8_t* out,
                                                    uint32_t* outLen,
                                                    uint32_t* out_result) {
    if (!validate_reply_invoke_params(ctx, out, outLen, out_result)) {
        return CALLBACK_ERR_INVALID_PARAM;
    }

    AsyncReplyContentFunc reply_func = nullptr;

    {
        std::lock_guard<std::mutex> lock(callback_registry_mutex_);

        auto it = callback_map_.find(port);
        if (it == callback_map_.end()) {
            return CALLBACK_ERR_STRATEGY_NOT_FOUND;
        }

        reply_func = it->second.reply;
    }

    uint32_t result = reply_func(ctx, out, outLen);
    *out_result = result;
    return CALLBACK_SUCCESS;
}

bool CallbackStrategyManager::validate_strategy(const CallbackStrategy* strategy) const {
    if (strategy == nullptr) {
        return false;
    }
    if (strategy->name == nullptr) {
        return false;
    }
    // 检查name字符串是否为空字符串
    // 注意：调用者必须保证name是有效的以'\0'结尾的C字符串
    if (strategy->name[0] == '\0') {
        return false;
    }
    // 检查端口范围（1-65535）
    if (strategy->port == 0) {
        return false;
    }
    if (strategy->parse == nullptr) {
        return false;
    }
    if (strategy->reply == nullptr) {
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

const CallbackStrategy* callback_registry_get_strategy(CallbackRegistry* registry,
                                                        uint16_t port) {
    if (registry == nullptr) {
        return nullptr;
    }
    auto* manager = reinterpret_cast<https_server_sim::CallbackStrategyManager*>(registry);
    return manager->get_callback(port);
}

// 文件结束
