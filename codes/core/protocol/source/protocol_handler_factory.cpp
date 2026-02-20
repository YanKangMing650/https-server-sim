// =============================================================================
//  HTTPS Server Simulator - Protocol Module
//  文件: protocol_handler_factory.cpp
//  描述: ProtocolHandlerFactory类实现 - 协议处理器工厂
//  版权: Copyright (c) 2026
// =============================================================================

#include "protocol/protocol_handler_factory.hpp"
#include "config/config.hpp"
#include "protocol/protocol_handler.hpp"

namespace https_server_sim {
namespace protocol {

std::unique_ptr<ProtocolHandler> ProtocolHandlerFactory::create(const config::Config& config) {
    // TODO: 当前简化版本：无论HTTP/2是否启用都返回Http1Handler
    // 后续版本完整实现HTTP/2后，需根据config.get_http2().enabled和ALPN协商结果
    // 选择返回Http1Handler或Http2Handler
    if (config.get_http2().enabled) {
        // HTTP/2启用：暂返回Http1Handler（当前主要实现）
        return std::make_unique<Http1Handler>();
    } else {
        // HTTP/2禁用：仅使用HTTP/1.1
        return std::make_unique<Http1Handler>();
    }
}

} // namespace protocol
} // namespace https_server_sim

// 文件结束
