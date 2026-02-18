#pragma once

#include <cstdint>
#include <vector>
#include "debug_chain/debug_config.hpp"

namespace https_server_sim {
namespace debug_chain {

// DebugContext结构体 - 存储调测上下文信息
struct DebugContext {
    DebugConfig config;                     // 调测配置
    std::vector<uint8_t> request_data;      // 请求数据
    std::vector<uint8_t> response_data;     // 响应数据
    uint16_t override_http_status;          // 覆盖的HTTP状态码（默认0）
    bool skip_callback;                      // 是否跳过回调（默认false）
    bool disconnect_after;                   // 处理后是否断开连接（默认false）

    // 默认构造函数
    DebugContext();
};

} // namespace debug_chain
} // namespace https_server_sim
