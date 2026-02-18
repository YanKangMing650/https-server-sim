// =============================================================================
//  HTTPS Server Simulator - Protocol Module
//  文件: http_message.hpp
//  描述: HttpRequest和HttpResponse类定义
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include "protocol/protocol_types.hpp"
#include <string>
#include <vector>
#include <map>

namespace https_server_sim {
namespace protocol {

// ==================== HTTP请求类 ====================
class HttpRequest {
public:
    /**
     * @brief 默认构造函数
     */
    HttpRequest();

    /**
     * @brief 重置请求对象到初始状态
     */
    void reset();

    /**
     * @brief 清空请求数据（同reset）
     */
    void clear();

    // 公开属性
    std::string method;
    std::string path;
    std::string version;
    HttpHeaders headers;
    std::vector<uint8_t> body;
    size_t content_length;
    std::string debug_token;
};

// ==================== HTTP响应类 ====================
class HttpResponse {
public:
    /**
     * @brief 默认构造函数
     */
    HttpResponse();

    /**
     * @brief 重置响应对象到初始状态
     */
    void reset();

    /**
     * @brief 清空响应数据（同reset）
     */
    void clear();

    /**
     * @brief 设置状态码和状态文本
     * @param code HTTP状态码
     * @param text 状态文本
     */
    void set_status(int code, const std::string& text);

    /**
     * @brief 添加响应头
     * @param name 头部名称
     * @param value 头部值
     */
    void add_header(const std::string& name, const std::string& value);

    /**
     * @brief 设置响应体
     * @param data 数据指针
     * @param len 数据长度
     */
    void set_body(const uint8_t* data, size_t len);

    // 公开属性
    int status_code;
    std::string status_text;
    HttpHeaders headers;
    std::vector<uint8_t> body;
};

} // namespace protocol
} // namespace https_server_sim

// 文件结束
