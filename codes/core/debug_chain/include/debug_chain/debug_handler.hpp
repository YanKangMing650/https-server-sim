#pragma once

#include <memory>
#include "debug_chain/debug_context.hpp"

namespace https_server_sim {
namespace debug_chain {

class DebugHandler {
public:
    virtual ~DebugHandler() = default;
    virtual const std::string& get_name() const = 0;
    virtual bool handle(DebugContext& context) = 0;

    void set_next(std::shared_ptr<DebugHandler> next);
    std::shared_ptr<DebugHandler> get_next() const;

protected:
    std::shared_ptr<DebugHandler> next_;
};

class DelayHandler : public DebugHandler {
public:
    DelayHandler();
    ~DelayHandler() override;
    const std::string& get_name() const override;
    bool handle(DebugContext& context) override;
};

class DisconnectHandler : public DebugHandler {
public:
    DisconnectHandler();
    ~DisconnectHandler() override;
    const std::string& get_name() const override;
    bool handle(DebugContext& context) override;
};

class ErrorCodeHandler : public DebugHandler {
public:
    ErrorCodeHandler();
    ~ErrorCodeHandler() override;
    const std::string& get_name() const override;
    bool handle(DebugContext& context) override;
};

class LogHandler : public DebugHandler {
public:
    LogHandler();
    ~LogHandler() override;
    const std::string& get_name() const override;
    bool handle(DebugContext& context) override;
};

} // namespace debug_chain
} // namespace https_server_sim
