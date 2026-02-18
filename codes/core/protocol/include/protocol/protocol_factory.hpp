// =============================================================================
//  HTTPS Server Simulator - Protocol Module
//  文件: protocol_factory.hpp
//  描述: ProtocolFactory类定义 - 协议处理器工厂
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include "protocol/protocol_types.hpp"
#include "protocol/protocol_handler.hpp"
#include <string>

namespace https_server_sim {

// 前向声明Connection
class Connection;

namespace protocol {

// ==================== 协议工厂类 ====================
class ProtocolFactory {
public:
    /**
     * @brief 获取工厂单例
     * @return ProtocolFactory引用
     */
    static ProtocolFactory& instance();

    /**
     * @brief 创建协议处理器
     * @param type 协议类型
     * @return ProtocolHandler指针，失败返回nullptr
     */
    ProtocolHandler* create_handler(ProtocolType type);

    /**
     * @brief 销毁协议处理器
     * @param handler 处理器指针
     */
    void destroy_handler(ProtocolHandler* handler);

    /**
     * @brief 根据ALPN选择协议类型
     * @param alpn ALPN协议字符串
     * @return ProtocolType
     */
    ProtocolType select_protocol_by_alpn(const std::string& alpn);

private:
    ProtocolFactory();
    ~ProtocolFactory();
    ProtocolFactory(const ProtocolFactory&) = delete;
    ProtocolFactory& operator=(const ProtocolFactory&) = delete;
};

} // namespace protocol
} // namespace https_server_sim

// 文件结束
