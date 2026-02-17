#include "protocol/protocol_handler.hpp"

namespace https_server_sim {
namespace protocol {

// Http1Handler
const std::string& Http1Handler::get_name() const {
    static const std::string name = "HTTP/1.1";
    return name;
}

void Http1Handler::close() {
}

// Http2Handler
const std::string& Http2Handler::get_name() const {
    static const std::string name = "HTTP/2";
    return name;
}

void Http2Handler::close() {
}

// ProtocolHandlerFactory
ProtocolHandlerFactory& ProtocolHandlerFactory::instance() {
    static ProtocolHandlerFactory factory;
    return factory;
}

ProtocolHandlerFactory::ProtocolHandlerFactory() {
}

ProtocolHandlerFactory::~ProtocolHandlerFactory() {
}

std::unique_ptr<TlsHandler> ProtocolHandlerFactory::create_tls_handler() {
    // 暂未实现TLS
    return nullptr;
}

std::unique_ptr<Http1Handler> ProtocolHandlerFactory::create_http1_handler() {
    return std::make_unique<Http1Handler>();
}

std::unique_ptr<Http2Handler> ProtocolHandlerFactory::create_http2_handler() {
    return std::make_unique<Http2Handler>();
}

} // namespace protocol
} // namespace https_server_sim
