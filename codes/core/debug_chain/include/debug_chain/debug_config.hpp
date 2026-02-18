// HTTPS Server 模拟器 - DebugConfig头文件
// 模块：DebugChain - 调测配置数据结构
// 用途：存储调测功能的配置参数
// 设计说明：
//   - 为保持C接口兼容性，结构体定义在全局命名空间
//   - 同时在https_server_sim::debug_chain命名空间提供类型别名
//   - 这种设计与项目其他模块风格保持一致

#pragma once

#include <cstdint>

// 全局命名空间中的DebugConfig（用于C接口兼容性）
struct DebugConfig {
    bool enabled;              // 总开关（默认false）
    uint32_t delay_ms;         // 延迟时间（毫秒，默认0）
    bool force_disconnect;     // 强制断开连接（默认false）
    bool log_packet;           // 记录请求/响应报文（默认false）
    uint16_t http_status;      // 返回的HTTP状态码（默认200）

#ifdef __cplusplus
    // C++默认构造函数 - 提供默认值初始化
    DebugConfig()
        : enabled(false)
        , delay_ms(0)
        , force_disconnect(false)
        , log_packet(false)
        , http_status(200)
    {
    }
#endif
};

#ifdef __cplusplus

namespace https_server_sim {
namespace debug_chain {

// 类型别名 - 将全局DebugConfig引入命名空间
// 这样C++代码可以使用https_server_sim::debug_chain::DebugConfig
using DebugConfig = ::DebugConfig;

// 初始化辅助函数（用于重置配置）
inline void InitDebugConfig(DebugConfig* config) {
    if (config != nullptr) {
        config->enabled = false;
        config->delay_ms = 0;
        config->force_disconnect = false;
        config->log_packet = false;
        config->http_status = 200;
    }
}

} // namespace debug_chain
} // namespace https_server_sim

#endif

// 模块：DebugChain - 调测配置数据结构
// 用途：定义DebugConfig结构体，提供调测功能的配置存储
