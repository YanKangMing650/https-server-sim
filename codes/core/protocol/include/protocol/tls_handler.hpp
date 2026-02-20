// =============================================================================
//  HTTPS Server Simulator - Protocol Module
//  文件: tls_handler.hpp
//  描述: TlsHandler类定义 - TLS防腐层
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include "protocol/protocol_types.hpp"
#include "utils/buffer.hpp"
#include <string>

// 统一使用void*避免条件编译带来的类型不一致
namespace https_server_sim {

// 前向声明Connection
class Connection;

namespace protocol {

// ==================== TLS处理器类（防腐层）====================
class TlsHandler {
public:
    /**
     * @brief 默认构造函数
     */
    TlsHandler();

    /**
     * @brief 析构函数
     */
    ~TlsHandler();

    // 禁止拷贝
    TlsHandler(const TlsHandler&) = delete;
    TlsHandler& operator=(const TlsHandler&) = delete;

    /**
     * @brief 初始化TLS处理器
     * @param conn Connection对象指针
     * @param cert_config 证书配置
     * @param tls_config TLS配置
     * @return 0成功，负数失败
     */
    int init(Connection* conn,
             const CertConfig& cert_config,
             const TlsConfig& tls_config);

    /**
     * @brief 读取解密后的明文数据
     * @param data 输出缓冲区
     * @param len 缓冲区长度
     * @param out_len 输出实际读取长度
     * @return 0成功，-EAGAIN需要更多数据，负数失败
     */
    int read(uint8_t* data, size_t len, size_t* out_len);

    /**
     * @brief 写入明文数据（会加密）
     * @param data 输入数据
     * @param len 数据长度
     * @param out_len 输出实际写入长度
     * @return 0成功，-EAGAIN需要更多数据，负数失败
     */
    int write(const uint8_t* data, size_t len, size_t* out_len);

    /**
     * @brief 关闭TLS处理器
     * @return 0成功，负数失败
     */
    int close();

    /**
     * @brief 检查TLS握手是否完成
     * @return true完成，false未完成
     */
    bool is_handshake_done() const;

    /**
     * @brief 继续TLS握手
     * @return 0握手完成，-EAGAIN需要继续，负数失败
     */
    int continue_handshake();

    /**
     * @brief 获取协商的ALPN协议
     * @return ALPN协议字符串，空表示未协商
     */
    std::string get_selected_alpn() const;

    /**
     * @brief 关闭TLS连接
     * @return 0成功，负数失败
     */
    int shutdown();

    /**
     * @brief 设置错误信息
     * @param code 错误码
     * @param msg 错误信息
     */
    void set_error(int code, const std::string& msg);

    /**
     * @brief 获取错误码
     * @return 错误码
     */
    int get_error_code() const;

    /**
     * @brief 获取错误信息
     * @return 错误信息
     */
    const std::string& get_error_msg() const;

    /**
     * @brief 重置TLS状态
     */
    void reset();

    /**
     * @brief 设置读缓冲区
     * @param buffer 缓冲区指针
     */
    void set_read_buffer(utils::Buffer* buffer);

    /**
     * @brief 设置写缓冲区
     * @param buffer 缓冲区指针
     */
    void set_write_buffer(utils::Buffer* buffer);

private:
    /**
     * @brief 初始化SSL上下文
     * @param config 证书配置
     * @return 0成功，负数失败
     */
    int init_ssl_context(const CertConfig& config);

    /**
     * @brief 加载证书
     * @param config 证书配置
     * @return 0成功，负数失败
     */
    int load_certificates(const CertConfig& config);

    /**
     * @brief 加载CA证书
     * @param ca_path CA证书路径
     * @return 0成功，负数失败
     */
    int load_ca_certificate(const std::string& ca_path);

    /**
     * @brief 配置证书
     * @param config 证书配置
     * @return 0成功，负数失败
     */
    int configure_certificates(const CertConfig& config);

    /**
     * @brief 配置加密套件
     * @param config TLS配置
     * @return 0成功，负数失败
     */
    int configure_cipher_suites(const TlsConfig& config);

    /**
     * @brief 配置ALPN
     * @return 0成功，负数失败
     */
    int configure_alpn();

    /**
     * @brief 设置内存BIO
     * @return 0成功，负数失败
     */
    int setup_bio();

    /**
     * @brief 从Connection读缓冲区读取数据写入SSL读BIO
     * @return 写入字节数，负数失败
     */
    int pump_read_bio();

    /**
     * @brief 从SSL写BIO读取数据写入Connection写缓冲区
     * @return 读取字节数，负数失败
     */
    int pump_write_bio();

    /**
     * @brief 检查是否是SM2证书
     * @return true是SM2，false不是
     */
    bool is_sm2_cert() const;

    /**
     * @brief 获取配置的ALPN协议列表
     * @return ALPN协议字符串
     */
    const std::string& get_alpn_protocols() const;

    Connection* conn_;
    // 统一使用void*避免条件编译的类型差异
    void* ssl_ctx_;
    void* ssl_;
    void* ssl_read_bio_;
    void* ssl_write_bio_;
    bool initialized_;
    bool handshake_done_;
    utils::Buffer* read_buffer_;
    utils::Buffer* write_buffer_;
    std::string cert_path_;
    std::string key_path_;
    std::string ca_path_;
    CertType cert_type_;
    bool use_gmssl_;
    int error_code_;
    std::string error_msg_;
    std::string alpn_protocols_;
};

} // namespace protocol
} // namespace https_server_sim

// 文件结束
