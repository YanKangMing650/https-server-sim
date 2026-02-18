// HTTPS Server 模拟器 - DebugContext头文件
// 模块：DebugChain - 调测上下文数据结构
// 用途：存储调测过程中的上下文信息
// 设计说明：
//   - 为保持C接口兼容性，结构体定义在全局命名空间
//   - 同时在https_server_sim::debug_chain命名空间提供类型别名
//   - 这种设计与项目其他模块风格保持一致

#pragma once

#include <cstdint>
#include <vector>
#include "debug_chain/debug_config.hpp"

// 全局命名空间中的DebugContext（用于C接口兼容性）
struct DebugContext {
    DebugConfig config;                     // 调测配置
    std::vector<uint8_t> request_data;      // 请求数据
    std::vector<uint8_t> response_data;     // 响应数据
    uint16_t override_http_status;          // 覆盖的HTTP状态码（默认0）
    bool skip_callback;                      // 是否跳过回调（默认false）
    bool disconnect_after;                   // 处理后是否断开连接（默认false）

    // 默认构造函数
    DebugContext()
        : override_http_status(0)
        , skip_callback(false)
        , disconnect_after(false)
    {
    }
};

#ifdef __cplusplus

namespace https_server_sim {
namespace debug_chain {

// 类型别名 - 将全局DebugContext引入命名空间
// 这样C++代码可以使用https_server_sim::debug_chain::DebugContext
using DebugContext = ::DebugContext;

} // namespace debug_chain
} // namespace https_server_sim

#endif

// 模块：DebugChain - 调测上下文数据结构
// 用途：定义DebugContext结构体，存储调测过程中的上下文信息
