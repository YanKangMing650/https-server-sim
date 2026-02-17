#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace https_server_sim {
namespace debug_chain {

enum class DebugActionType {
    NONE,
    DELAY,
    DISCONNECT,
    RETURN_ERROR,
    LOG
};

struct DebugPointConfig {
    std::string name;
    uint16_t server_port;
    DebugActionType action;
    uint32_t delay_ms;
    int32_t error_code;
    uint32_t probability;
    bool enabled;

    DebugPointConfig();
};

class DebugChainConfig {
public:
    DebugChainConfig();
    ~DebugChainConfig();

    void set_enabled(bool enabled);
    bool is_enabled() const;

    void add_point(const DebugPointConfig& point);
    void remove_point(const std::string& name);
    void remove_point(uint16_t server_port, const std::string& name);

    const DebugPointConfig* get_point(const std::string& name) const;
    const DebugPointConfig* get_point(uint16_t server_port, const std::string& name) const;
    std::vector<const DebugPointConfig*> get_points_for_port(uint16_t server_port) const;
    const std::vector<DebugPointConfig>& get_all_points() const;

    void clear();
    void reset();

private:
    bool enabled_;
    std::vector<DebugPointConfig> points_;
};

} // namespace debug_chain
} // namespace https_server_sim
