// =============================================================================
//  HTTPS Server Simulator - Protocol Module
//  文件: http_parser.hpp
//  描述: HttpParser类定义 - HTTP/1.1解析工具类
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include "protocol/protocol_types.hpp"
#include "protocol/http_message.hpp"
#include "utils/buffer.hpp"
#include <string>
#include <map>

namespace https_server_sim {
namespace protocol {

// ==================== HTTP解析器类 ====================
class HttpParser {
public:
    /**
     * @brief 默认构造函数
     */
    HttpParser();

    /**
     * @brief 析构函数
     */
    ~HttpParser();

    /**
     * @brief 初始化解析器，绑定缓冲区
     * @param buffer 缓冲区指针
     */
    void init(utils::Buffer* buffer);

    /**
     * @brief 从缓冲区读取一行（以\r\n结尾）
     * @param out 输出缓冲区
     * @param max_len 输出缓冲区最大长度
     * @param out_len 输出实际读取长度
     * @return 0成功，-EAGAIN需要更多数据，负数失败
     */
    int read_line(char* out, size_t max_len, size_t* out_len);

    /**
     * @brief 解析HTTP请求行
     * @param line 请求行字符串
     * @param len 请求行长度
     * @param method 输出方法
     * @param path 输出路径
     * @param version 输出版本
     * @return 0成功，负数失败
     */
    int parse_request_line(const char* line, size_t len,
                           std::string* method,
                           std::string* path,
                           std::string* version);

    /**
     * @brief 解析所有HTTP头部直到空行
     * @param headers 输出头部集合
     * @return 0成功，-EAGAIN需要更多数据，负数失败
     */
    int parse_headers(std::map<std::string, std::string>* headers);

    /**
     * @brief 解析单条HTTP头部行
     * @param line 头部行字符串
     * @param len 头部行长度
     * @param key 输出头部名称
     * @param value 输出头部值
     * @return 0成功，负数失败
     */
    int parse_header(const char* line, size_t len,
                     std::string* key,
                     std::string* value);

    /**
     * @brief 查找指定HTTP头部（大小写不敏感）
     * @param headers 头部集合
     * @param name 头部名称
     * @param value 输出头部值（可为nullptr）
     * @return true找到，false未找到
     */
    bool find_header(const std::map<std::string, std::string>& headers,
                     const std::string& name,
                     std::string* value);

    /**
     * @brief 构建HTTP响应报文
     * @param resp 响应对象
     * @param out 输出缓冲区
     * @param max_len 输出缓冲区最大长度
     * @param out_len 输出实际长度
     * @return 0成功，负数失败
     */
    int build_response(const HttpResponse& resp,
                       uint8_t* out, size_t max_len,
                       size_t* out_len);

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
     * @brief 重置解析器状态
     */
    void reset();

private:
    utils::Buffer* buffer_;
    int error_code_;
    std::string error_msg_;
};

} // namespace protocol
} // namespace https_server_sim

// 文件结束
