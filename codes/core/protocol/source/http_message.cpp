// =============================================================================
//  HTTPS Server Simulator - Protocol Module
//  文件: http_message.cpp
//  描述: HttpRequest和HttpResponse类实现
//  版权: Copyright (c) 2026
// =============================================================================
#include "protocol/http_message.hpp"
#include <algorithm>

namespace https_server_sim {
namespace protocol {

// ==================== HttpRequest实现 ====================

HttpRequest::HttpRequest()
    : method()
    , path()
    , version("HTTP/1.1")
    , headers()
    , body()
    , content_length(0)
    , debug_token()
{
}

void HttpRequest::reset() {
    method.clear();
    path.clear();
    version = "HTTP/1.1";
    headers.clear();
    body.clear();
    content_length = 0;
    debug_token.clear();
}

void HttpRequest::clear() {
    reset();
}

// ==================== HttpResponse实现 ====================

HttpResponse::HttpResponse()
    : status_code(200)
    , status_text("OK")
    , headers()
    , body()
{
}

void HttpResponse::reset() {
    status_code = 200;
    status_text = "OK";
    headers.clear();
    body.clear();
}

void HttpResponse::clear() {
    reset();
}

void HttpResponse::set_status(int code, const std::string& text) {
    status_code = code;
    status_text = text;
}

void HttpResponse::add_header(const std::string& name, const std::string& value) {
    headers[name] = value;
}

void HttpResponse::set_body(const uint8_t* data, size_t len) {
    body.resize(len);
    if (len > 0 && data != nullptr) {
        std::copy(data, data + len, body.begin());
    }
}

} // namespace protocol
} // namespace https_server_sim

// 文件结束
