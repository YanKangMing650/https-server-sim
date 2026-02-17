#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <functional>
#include "callback/client_context.hpp"

namespace https_server_sim {
namespace callback {

// 回调策略接口
class CallbackStrategy {
public:
    virtual ~CallbackStrategy() = default;
    virtual const std::string& get_name() const = 0;
    virtual bool execute(ClientContext& context) = 0;
};

// 默认回调策略
class DefaultCallbackStrategy : public CallbackStrategy {
public:
    DefaultCallbackStrategy();
    ~DefaultCallbackStrategy() override;

    const std::string& get_name() const override;
    bool execute(ClientContext& context) override;
};

// 回调管理器
class CallbackManager {
public:
    CallbackManager();
    ~CallbackManager();

    void set_callbacks_dir(const std::string& dir);
    const std::string& get_callbacks_dir() const;

    bool load_script(const std::string& script_path, uint16_t server_port);
    void unload_script(uint16_t server_port);

    CallbackStrategy* get_strategy(uint16_t server_port);
    bool execute_callback(ClientContext& context);

    void register_strategy(uint16_t server_port, std::unique_ptr<CallbackStrategy> strategy);

private:
    std::string callbacks_dir_;
    std::unordered_map<uint16_t, std::unique_ptr<CallbackStrategy>> strategies_;
    DefaultCallbackStrategy default_strategy_;
};

} // namespace callback
} // namespace https_server_sim
