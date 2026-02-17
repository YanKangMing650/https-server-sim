#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace https_server_sim {
namespace debug_chain {

class DebugContext {
public:
    DebugContext();
    ~DebugContext();

    uint64_t get_connection_id() const;
    void set_connection_id(uint64_t id);

    uint16_t get_server_port() const;
    void set_server_port(uint16_t port);

    const std::string& get_point_name() const;
    void set_point_name(const std::string& name);

    bool is_skipped() const;
    void set_skipped(bool skipped);

    int32_t get_error_code() const;
    void set_error_code(int32_t code);

    uint32_t get_delay_ms() const;
    void set_delay_ms(uint32_t ms);

    void add_log(const std::string& message);
    const std::vector<std::string>& get_logs() const;

    void reset();

private:
    uint64_t connection_id_;
    uint16_t server_port_;
    std::string point_name_;
    bool skipped_;
    int32_t error_code_;
    uint32_t delay_ms_;
    std::vector<std::string> logs_;
};

} // namespace debug_chain
} // namespace https_server_sim
