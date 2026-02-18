// =============================================================================
//  HTTPS Server Simulator - Protocol Module
//  文件: protocol_handler.hpp
//  描述: ProtocolHandler接口及实现类定义
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include "protocol/protocol_types.hpp"
#include "protocol/http_message.hpp"
#include "protocol/http_parser.hpp"
#include "protocol/http2_stream.hpp"
#include "protocol/hpack.hpp"
#include "protocol/tls_handler.hpp"
#include "utils/buffer.hpp"
#include <string>
#include <map>
#include <memory>
#include <vector>

namespace https_server_sim {

// 前向声明Connection
class Connection;

namespace protocol {

// ==================== 协议处理器接口 ====================
class ProtocolHandler {
public:
    /**
     * @brief 虚析构函数
     */
    virtual ~ProtocolHandler() = default;

    /**
     * @brief 初始化协议处理器
     * @param conn Connection对象指针
     * @param cert_config 证书配置
     * @param tls_config TLS配置
     * @return 0成功，负数失败
     */
    virtual int init(Connection* conn,
                     const CertConfig& cert_config,
                     const TlsConfig& tls_config) = 0;

    /**
     * @brief 处理读事件（解析协议数据）
     * @return 0成功，负数失败
     */
    virtual int on_read() = 0;

    /**
     * @brief 处理写事件（发送协议数据）
     * @return 0成功，负数失败
     */
    virtual int on_write() = 0;

    /**
     * @brief 发送响应数据
     * @param data 数据指针
     * @param len 数据长度
     * @return 0成功，负数失败
     */
    virtual int send_response(const uint8_t* data, uint32_t len) = 0;

    /**
     * @brief 关闭协议处理器
     */
    virtual void close() = 0;

    /**
     * @brief 获取TLS处理器
     * @return TlsHandler指针
     */
    virtual TlsHandler* get_tls_handler() = 0;

    /**
     * @brief 获取协议类型
     * @return ProtocolType
     */
    virtual ProtocolType get_protocol_type() const = 0;

    /**
     * @brief 重置协议处理器状态
     */
    virtual void reset() = 0;
};

// ==================== HTTP/1.1协议处理器 ====================
class Http1Handler : public ProtocolHandler {
public:
    /**
     * @brief 默认构造函数
     */
    Http1Handler();

    /**
     * @brief 析构函数
     */
    ~Http1Handler() override;

    // 禁止拷贝
    Http1Handler(const Http1Handler&) = delete;
    Http1Handler& operator=(const Http1Handler&) = delete;

    /**
     * @brief 初始化HTTP/1.1处理器
     */
    int init(Connection* conn,
             const CertConfig& cert_config,
             const TlsConfig& tls_config) override;

    /**
     * @brief 处理读事件
     */
    int on_read() override;

    /**
     * @brief 处理写事件
     */
    int on_write() override;

    /**
     * @brief 发送响应数据
     */
    int send_response(const uint8_t* data, uint32_t len) override;

    /**
     * @brief 关闭处理器
     */
    void close() override;

    /**
     * @brief 获取TLS处理器
     */
    TlsHandler* get_tls_handler() override;

    /**
     * @brief 获取协议类型
     */
    ProtocolType get_protocol_type() const override;

    /**
     * @brief 重置状态
     */
    void reset() override;

private:
    /**
     * @brief 解析请求体
     * @return 0成功，负数失败
     */
    int parse_body();

    /**
     * @brief 处理完整请求
     * @return 0成功，负数失败
     */
    int handle_complete_request();

    /**
     * @brief 生成响应
     * @return 0成功，负数失败
     */
    int generate_response();

    Connection* conn_;
    std::unique_ptr<TlsHandler> tls_handler_;
    Http1ParseState state_;
    HttpRequest request_;
    HttpResponse response_;
    utils::Buffer* read_buffer_;
    utils::Buffer* write_buffer_;
    std::unique_ptr<utils::Buffer> plaintext_buffer_;
    HttpParser parser_;
};

// ==================== HTTP/2协议处理器 ====================
class Http2Handler : public ProtocolHandler {
public:
    /**
     * @brief 默认构造函数
     */
    Http2Handler();

    /**
     * @brief 析构函数
     */
    ~Http2Handler() override;

    // 禁止拷贝
    Http2Handler(const Http2Handler&) = delete;
    Http2Handler& operator=(const Http2Handler&) = delete;

    /**
     * @brief 初始化HTTP/2处理器
     */
    int init(Connection* conn,
             const CertConfig& cert_config,
             const TlsConfig& tls_config) override;

    /**
     * @brief 处理读事件
     */
    int on_read() override;

    /**
     * @brief 处理写事件
     */
    int on_write() override;

    /**
     * @brief 发送响应数据
     */
    int send_response(const uint8_t* data, uint32_t len) override;

    /**
     * @brief 关闭处理器
     */
    void close() override;

    /**
     * @brief 获取TLS处理器
     */
    TlsHandler* get_tls_handler() override;

    /**
     * @brief 获取协议类型
     */
    ProtocolType get_protocol_type() const override;

    /**
     * @brief 重置状态
     */
    void reset() override;

private:
    /**
     * @brief 读取HTTP/2帧
     * @return 0成功，-EAGAIN需要更多数据，负数失败
     */
    int read_frame();

    /**
     * @brief 处理HTTP/2帧
     * @param header 帧头
     * @param payload 帧数据
     * @return 0成功，负数失败
     */
    int handle_frame(const Http2FrameHeader* header, const uint8_t* payload);

    /**
     * @brief 处理SETTINGS帧
     * @param payload 帧数据
     * @param len 数据长度
     * @return 0成功，负数失败
     */
    int handle_settings_frame(const uint8_t* payload, size_t len);

    /**
     * @brief 处理HEADERS帧
     * @param stream_id 流ID
     * @param payload 帧数据
     * @param len 数据长度
     * @param flags 帧标志
     * @return 0成功，负数失败
     */
    int handle_headers_frame(uint32_t stream_id, const uint8_t* payload,
                             size_t len, uint8_t flags);

    /**
     * @brief 处理DATA帧
     * @param stream_id 流ID
     * @param payload 帧数据
     * @param len 数据长度
     * @param flags 帧标志
     * @return 0成功，负数失败
     */
    int handle_data_frame(uint32_t stream_id, const uint8_t* payload,
                          size_t len, uint8_t flags);

    /**
     * @brief 处理CONTINUATION帧
     * @param stream_id 流ID
     * @param payload 帧数据
     * @param len 数据长度
     * @param flags 帧标志
     * @return 0成功，负数失败
     */
    int handle_continuation_frame(uint32_t stream_id, const uint8_t* payload,
                                  size_t len, uint8_t flags);

    /**
     * @brief 处理WINDOW_UPDATE帧
     * @param stream_id 流ID
     * @param increment 窗口增量
     * @return 0成功，负数失败
     */
    int handle_window_update_frame(uint32_t stream_id, uint32_t increment);

    /**
     * @brief 处理RST_STREAM帧
     * @param stream_id 流ID
     * @param error_code 错误码
     * @return 0成功，负数失败
     */
    int handle_rst_stream_frame(uint32_t stream_id, uint32_t error_code);

    /**
     * @brief 处理PING帧
     * @param payload 帧数据
     * @param len 数据长度
     * @return 0成功，负数失败
     */
    int handle_ping_frame(const uint8_t* payload, size_t len);

    /**
     * @brief 处理GOAWAY帧
     * @param payload 帧数据
     * @param len 数据长度
     * @return 0成功，负数失败
     */
    int handle_goaway_frame(const uint8_t* payload, size_t len);

    /**
     * @brief 发送SETTINGS帧
     * @return 0成功，负数失败
     */
    int send_settings_frame();

    /**
     * @brief 发送SETTINGS ACK
     * @return 0成功，负数失败
     */
    int send_settings_ack();

    /**
     * @brief 发送HEADERS帧
     * @param stream_id 流ID
     * @param headers 头部集合
     * @param end_stream 是否结束流
     * @return 0成功，负数失败
     */
    int send_headers_frame(uint32_t stream_id,
                           const std::map<std::string, std::string>& headers,
                           bool end_stream);

    /**
     * @brief 发送DATA帧
     * @param stream_id 流ID
     * @param data 数据
     * @param len 数据长度
     * @param end_stream 是否结束流
     * @return 0成功，负数失败
     */
    int send_data_frame(uint32_t stream_id, const uint8_t* data,
                        size_t len, bool end_stream);

    /**
     * @brief 发送WINDOW_UPDATE帧
     * @param stream_id 流ID
     * @param increment 窗口增量
     * @return 0成功，负数失败
     */
    int send_window_update_frame(uint32_t stream_id, uint32_t increment);

    /**
     * @brief 发送RST_STREAM帧
     * @param stream_id 流ID
     * @param error_code 错误码
     * @return 0成功，负数失败
     */
    int send_rst_stream_frame(uint32_t stream_id, uint32_t error_code);

    /**
     * @brief 写入HTTP/2帧
     * @param type 帧类型
     * @param stream_id 流ID
     * @param flags 帧标志
     * @param payload 帧数据
     * @param len 数据长度
     * @return 0成功，负数失败
     */
    int write_frame(Http2FrameType type, uint32_t stream_id,
                    uint8_t flags, const uint8_t* payload, size_t len);

    /**
     * @brief 获取或创建流
     * @param stream_id 流ID
     * @return Http2Stream指针
     */
    Http2Stream* get_or_create_stream(uint32_t stream_id);

    /**
     * @brief 处理完整请求
     * @param stream 流对象
     * @return 0成功，负数失败
     */
    int handle_complete_request(Http2Stream* stream);

    Connection* conn_;
    std::unique_ptr<TlsHandler> tls_handler_;
    std::map<uint32_t, std::unique_ptr<Http2Stream>> streams_;
    utils::Buffer* read_buffer_;
    utils::Buffer* write_buffer_;
    std::unique_ptr<utils::Buffer> plaintext_buffer_;
    Http2Settings settings_;
    Http2Settings local_settings_;
    uint32_t last_stream_id_;
    std::unique_ptr<HpackEncoder> hpack_encoder_;
    std::unique_ptr<HpackDecoder> hpack_decoder_;
    bool settings_ack_received_;
    bool settings_ack_sent_;
};

} // namespace protocol
} // namespace https_server_sim

// 文件结束
