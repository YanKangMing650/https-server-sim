#pragma once

#include <memory>
#include "debug_chain/debug_config.hpp"
#include "debug_chain/debug_context.hpp"
#include "debug_chain/debug_handler.hpp"

namespace https_server_sim {
namespace debug_chain {

class DebugChain {
public:
    DebugChain();
    ~DebugChain();

    void set_config(const DebugChainConfig& config);
    const DebugChainConfig& get_config() const;

    void set_enabled(bool enabled);
    bool is_enabled() const;

    bool execute(uint16_t server_port, const std::string& point_name, DebugContext& context);
    bool execute_simple(uint64_t connection_id, uint16_t server_port, const std::string& point_name);

private:
    void build_chain();
    bool should_trigger(uint32_t probability) const;

    DebugChainConfig config_;
    bool enabled_;
    std::shared_ptr<DebugHandler> chain_head_;
};

class DebugChainManager {
public:
    static DebugChainManager& instance();

    DebugChain& get_chain();
    bool execute(uint16_t server_port, const std::string& point_name, DebugContext& context);
    bool execute_simple(uint64_t connection_id, uint16_t server_port, const std::string& point_name);

private:
    DebugChainManager();
    ~DebugChainManager();
    DebugChainManager(const DebugChainManager&) = delete;
    DebugChainManager& operator=(const DebugChainManager&) = delete;

    DebugChain chain_;
};

} // namespace debug_chain
} // namespace https_server_sim
