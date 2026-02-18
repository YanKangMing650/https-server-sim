#pragma once

#include <cstdint>

namespace https_server_sim {
namespace debug_chain {

// DebugConfig结构体 - 存储调测配置
struct DebugConfig {
    bool enabled;              // 总开关（默认false）
    uint32_t delay_ms;         // 延迟时间（毫秒，默认0）
    bool force_disconnect;     // 强制断开连接（默认false）
    bool log_packet;           // 记录请求/响应报文（默认false）
    uint16_t http_status;      // 返回的HTTP状态码（默认200）

    // 默认构造函数
    DebugConfig();
};

} // namespace debug_chain
} // namespace https_server_sim
