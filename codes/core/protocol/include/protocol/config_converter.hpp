// =============================================================================
//  HTTPS Server Simulator - Protocol Module
//  文件: config_converter.hpp
//  描述: ConfigConverter类定义 - 配置转换器（Adaptor层）
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include "config/config.hpp"
#include "protocol/protocol_types.hpp"

namespace https_server_sim {
namespace protocol {

/**
 * @brief 配置转换器（Adaptor层）：Config模块配置 → Protocol模块配置
 *
 * 【模块边界】：本类仅负责配置结构转换，不包含任何协议处理逻辑
 *              - 不涉及TLS握手
 *              - 不涉及HTTP解析
 *              - 仅做字段映射和转换
 *
 * 职责：将config::Config的配置转换为protocol模块需要的格式
 * 设计理由：
 * - 解耦Server和配置转换逻辑
 * - 便于单元测试
 * - 符合单一职责原则
 */
class ConfigConverter {
public:
    /**
     * @brief 将CertificatesConfig转换为CertConfig
     * @param src Config模块的证书配置
     * @param dst Protocol模块的证书配置（输出）
     */
    static void convert_cert_config(
        const config::CertificatesConfig& src,
        CertConfig& dst);

    /**
     * @brief 从Config构造TlsConfig
     * @param config Config模块的主配置
     * @param dst Protocol模块的TLS配置（输出）
     */
    static void convert_tls_config(
        const config::Config& config,
        TlsConfig& dst);
};

} // namespace protocol
} // namespace https_server_sim

// 文件结束
