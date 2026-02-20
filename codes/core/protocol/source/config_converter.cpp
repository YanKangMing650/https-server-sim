// =============================================================================
//  HTTPS Server Simulator - Protocol Module
//  文件: config_converter.cpp
//  描述: ConfigConverter类实现 - 配置转换器
//  版权: Copyright (c) 2026
// =============================================================================

#include "protocol/config_converter.hpp"

namespace https_server_sim {
namespace protocol {

void ConfigConverter::convert_cert_config(
    const config::CertificatesConfig& src,
    CertConfig& dst)
{
    // 基础证书字段（直接映射）
    dst.cert_path = src.cert_path;
    dst.key_path = src.key_path;
    dst.ca_path = src.ca_path;

    // use_gmssl标志设置规则（按设计文档规定）：
    // - 只要配置了国密证书路径(sm2_cert_path)或国密私钥路径(sm2_key_path)中的任意一个，
    //   就认为需要启用国密支持
    // - 【设计说明】此逻辑为设计文档明确规定，当前版本不做完整性校验（如同时配置sm2_cert和sm2_key）
    // - 【注意】若只配置部分国密路径，后续国密初始化可能失败，这是预期行为
    dst.use_gmssl = !src.sm2_cert_path.empty() || !src.sm2_key_path.empty();

    // 国密字段处理说明：
    // - 当前protocol::CertConfig不包含sm2_*字段（除use_gmssl外）
    // - TODO: 国密支持需后续扩展CertConfig结构，添加sm2_cert_path、sm2_key_path等字段映射
    // - 当前版本暂不映射sm2_cert_path、sm2_key_path等
}

void ConfigConverter::convert_tls_config(
    const config::Config& config,
    TlsConfig& dst)
{
    const auto& cert_config = config.get_certificates();

    // 密码套件从CertificatesConfig::cipher_suite映射
    dst.cipher_suites = cert_config.cipher_suite;

    // ALPN协议：默认http/1.1，HTTP/2启用时可添加h2
    const auto& http2_config = config.get_http2();
    if (http2_config.enabled) {
        dst.alpn_protocols = ALPN_HTTP2_HTTP11;
    } else {
        dst.alpn_protocols = ALPN_HTTP11;
    }

    // TLS版本启用标志：使用默认值
    dst.enable_tls_1_3 = DEFAULT_ENABLE_TLS_1_3;
    dst.enable_tls_1_2 = DEFAULT_ENABLE_TLS_1_2;
}

} // namespace protocol
} // namespace https_server_sim

// 文件结束
