// =============================================================================
//  HTTPS Server Simulator - Protocol Module
//  文件: protocol_handler_factory.hpp
//  描述: ProtocolHandlerFactory类定义 - 协议处理器工厂
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include <memory>

namespace https_server_sim {
namespace config { class Config; }
namespace protocol { class ProtocolHandler; }
}

namespace https_server_sim {
namespace protocol {

/**
 * @brief ProtocolHandler工厂类
 *
 * 职责：根据配置创建合适的ProtocolHandler实例
 */
class ProtocolHandlerFactory {
public:
    /**
     * @brief 创建ProtocolHandler实例
     * @param config 配置引用，用于决定创建Http1Handler还是Http2Handler
     * @return unique_ptr<ProtocolHandler>，失败返回nullptr
     */
    static std::unique_ptr<ProtocolHandler> create(const config::Config& config);
};

} // namespace protocol
} // namespace https_server_sim

// 文件结束
