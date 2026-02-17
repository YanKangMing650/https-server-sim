#pragma once

#include <cstdint>
#include <string>
#include <memory>

namespace https_server_sim {
namespace protocol {

// 协议处理器接口
class ProtocolHandler {
public:
    virtual ~ProtocolHandler() = default;

    // 获取协议名称
    virtual const std::string& get_name() const = 0;

    // 关闭处理器
    virtual void close() = 0;
};

// TLS处理器接口
class TlsHandler : public ProtocolHandler {
public:
    ~TlsHandler() override = default;

    // 继续握手
    // return: true-握手完成，false-需要继续
    virtual bool continue_handshake() = 0;

    // 是否已完成握手
    virtual bool is_handshake_done() const = 0;
};

// HTTP/1.1处理器
class Http1Handler : public ProtocolHandler {
public:
    ~Http1Handler() override = default;

    const std::string& get_name() const override;
    void close() override;
};

// HTTP/2处理器
class Http2Handler : public ProtocolHandler {
public:
    ~Http2Handler() override = default;

    const std::string& get_name() const override;
    void close() override;
};

// 协议处理器工厂
class ProtocolHandlerFactory {
public:
    static ProtocolHandlerFactory& instance();

    // 创建TLS处理器
    std::unique_ptr<TlsHandler> create_tls_handler();

    // 创建HTTP/1.1处理器
    std::unique_ptr<Http1Handler> create_http1_handler();

    // 创建HTTP/2处理器
    std::unique_ptr<Http2Handler> create_http2_handler();

private:
    ProtocolHandlerFactory();
    ~ProtocolHandlerFactory();
    ProtocolHandlerFactory(const ProtocolHandlerFactory&) = delete;
    ProtocolHandlerFactory& operator=(const ProtocolHandlerFactory&) = delete;
};

} // namespace protocol
} // namespace https_server_sim
