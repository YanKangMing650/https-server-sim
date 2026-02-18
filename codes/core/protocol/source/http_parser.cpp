// =============================================================================
//  HTTPS Server Simulator - Protocol Module
//  文件: http_parser.cpp
//  描述: HttpParser类实现
//  版权: Copyright (c) 2026
// =============================================================================
#include "protocol/http_parser.hpp"
#include "protocol/protocol_utils.hpp"
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cerrno>

namespace https_server_sim {
namespace protocol {

// ==================== HttpParser实现 ====================

HttpParser::HttpParser()
    : buffer_(nullptr)
    , error_code_(0)
    , error_msg_()
{
}

HttpParser::~HttpParser() = default;

void HttpParser::init(utils::Buffer* buffer) {
    buffer_ = buffer;
    error_code_ = 0;
    error_msg_.clear();
}

int HttpParser::read_line(char* out, size_t max_len, size_t* out_len) {
    if (!buffer_) {
        set_error(PROTOCOL_ERROR_INVALID, "Buffer not initialized");
        return PROTOCOL_ERROR_INVALID;
    }

    size_t readable = buffer_->readable_bytes();
    if (readable == 0) {
        *out_len = 0;
        return PROTOCOL_ERROR_EAGAIN;
    }

    // 查找 "\r\n"
    const uint8_t* data = buffer_->read_ptr();
    size_t pos = 0;
    bool found = false;

    for (pos = 0; pos + 1 < readable; ++pos) {
        if (data[pos] == '\r' && data[pos + 1] == '\n') {
            found = true;
            break;
        }
    }

    if (!found) {
        *out_len = 0;
        return PROTOCOL_ERROR_EAGAIN;
    }

    // 检查输出缓冲区大小（留出null终止符空间）
    if (pos + 1 > max_len) {
        set_error(PROTOCOL_ERROR_TOO_LONG, "Line too long");
        return PROTOCOL_ERROR_TOO_LONG;
    }

    // 复制数据（不包含\r\n）
    if (pos > 0) {
        memcpy(out, data, pos);
    }
    out[pos] = '\0';
    *out_len = pos;

    // 消耗缓冲区数据（包含\r\n）
    buffer_->skip(pos + 2);

    return PROTOCOL_OK;
}

int HttpParser::parse_request_line(const char* line, size_t len,
                               std::string* method,
                               std::string* path,
                               std::string* version) {
    // 清空输出
    method->clear();
    path->clear();
    version->clear();

    // 按空格拆分三部分
    // 第一部分: method
    size_t pos = 0;
    while (pos < len && line[pos] != ' ') {
        pos++;
    }
    if (pos == 0 || pos >= len) {
        set_error(PROTOCOL_ERROR_INVALID, "Invalid request line format");
        return PROTOCOL_ERROR_INVALID;
    }
    *method = std::string(line, pos);

    // 跳过中间空格
    while (pos < len && line[pos] == ' ') {
        pos++;
    }
    if (pos >= len) {
        set_error(PROTOCOL_ERROR_INVALID, "Invalid request line format");
        return PROTOCOL_ERROR_INVALID;
    }

    // 第二部分: path
    size_t path_start = pos;
    while (pos < len && line[pos] != ' ') {
        pos++;
    }
    if (pos == path_start || pos >= len) {
        set_error(PROTOCOL_ERROR_INVALID, "Invalid request line format");
        return PROTOCOL_ERROR_INVALID;
    }
    *path = std::string(line + path_start, pos - path_start);

    // 跳过中间空格
    while (pos < len && line[pos] == ' ') {
        pos++;
    }
    if (pos >= len) {
        set_error(PROTOCOL_ERROR_INVALID, "Invalid request line format");
        return PROTOCOL_ERROR_INVALID;
    }

    // 第三部分: version
    *version = std::string(line + pos, len - pos);

    // 校验HTTP版本（仅支持HTTP/1.1）
    if (*version != "HTTP/1.1") {
        set_error(PROTOCOL_ERROR_VERSION, "HTTP version not supported");
        return PROTOCOL_ERROR_VERSION;
    }

    return PROTOCOL_OK;
}

int HttpParser::parse_headers(std::map<std::string, std::string>* headers) {
    if (!buffer_) {
        set_error(PROTOCOL_ERROR_INVALID, "Buffer not initialized");
        return PROTOCOL_ERROR_INVALID;
    }

    headers->clear();
    char line[MAX_HEADER_LINE_LEN];
    size_t line_len = 0;
    int header_count = 0;

    while (true) {
        int ret = read_line(line, sizeof(line), &line_len);
        if (ret != PROTOCOL_OK) {
            return ret;
        }

        // 空行表示头部结束
        if (line_len == 0) {
            break;
        }

        // 检查头部数量限制
        if (header_count >= static_cast<int>(MAX_HEADERS)) {
            set_error(PROTOCOL_ERROR_TOO_MANY, "Too many headers");
            return PROTOCOL_ERROR_TOO_MANY;
        }

        std::string key, value;
        ret = parse_header(line, line_len, &key, &value);
        if (ret != PROTOCOL_OK) {
            return ret;
        }

        (*headers)[key] = value;
        header_count++;
    }

    return PROTOCOL_OK;
}

int HttpParser::parse_header(const char* line, size_t len,
                              std::string* key,
                              std::string* value) {
    // 查找冒号分隔符
    const char* colon = nullptr;
    for (size_t i = 0; i < len; ++i) {
        if (line[i] == ':') {
            colon = line + i;
            break;
        }
    }

    if (!colon) {
        set_error(PROTOCOL_ERROR_INVALID, "Invalid header format");
        return PROTOCOL_ERROR_INVALID;
    }

    // 提取key（冒号之前，trim whitespace）
    const char* key_start = line;
    const char* key_end = colon;
    // 跳过前面的空格
    while (key_start < key_end && (*key_start == ' ' || *key_start == '\t')) {
        key_start++;
    }
    // 跳过后面的空格
    while (key_end > key_start && (*(key_end - 1) == ' ' || *(key_end - 1) == '\t')) {
        key_end--;
    }

    if (key_end - key_start == 0) {
        set_error(PROTOCOL_ERROR_INVALID, "Empty header name");
        return PROTOCOL_ERROR_INVALID;
    }

    if ((size_t)(key_end - key_start) > MAX_HEADER_NAME_LEN) {
        set_error(PROTOCOL_ERROR_TOO_LONG, "Header name too long");
        return PROTOCOL_ERROR_TOO_LONG;
    }

    *key = std::string(key_start, key_end - key_start);

    // 提取value（冒号之后，trim whitespace）
    const char* value_start = colon + 1;
    const char* value_end = line + len;
    while (value_start < value_end && (*value_start == ' ' || *value_start == '\t')) {
        value_start++;
    }
    while (value_end > value_start && (*(value_end - 1) == ' ' || *(value_end - 1) == '\t')) {
        value_end--;
    }

    if ((size_t)(value_end - value_start) > MAX_HEADER_VALUE_LEN) {
        set_error(PROTOCOL_ERROR_TOO_LONG, "Header value too long");
        return PROTOCOL_ERROR_TOO_LONG;
    }

    *value = std::string(value_start, value_end - value_start);

    return PROTOCOL_OK;
}

bool HttpParser::find_header(const std::map<std::string, std::string>& headers,
                              const std::string& name,
                              std::string* value) {
    // 大小写不敏感比较
    for (const auto& pair : headers) {
        if (StrCaseCmp(pair.first.c_str(), name.c_str()) == 0) {
            if (value) {
                *value = pair.second;
            }
            return true;
        }
    }
    return false;
}

int HttpParser::build_response(const HttpResponse& resp,
                                uint8_t* out, size_t max_len,
                                size_t* out_len) {
    size_t offset = 0;

    // 1. 写入状态行
    std::string status_line = "HTTP/1.1 " + std::to_string(resp.status_code) +
                              " " + resp.status_text + "\r\n";

    if (offset + status_line.size() > max_len) {
        set_error(PROTOCOL_ERROR_BUFFER, "Buffer too small");
        return PROTOCOL_ERROR_BUFFER;
    }
    memcpy(out + offset, status_line.data(), status_line.size());
    offset += status_line.size();

    // 2. 写入Headers，同时检查是否有Content-Length
    bool has_content_length = false;
    for (const auto& header : resp.headers) {
        std::string header_line = header.first + ": " + header.second + "\r\n";
        if (offset + header_line.size() > max_len) {
            set_error(PROTOCOL_ERROR_BUFFER, "Buffer too small");
            return PROTOCOL_ERROR_BUFFER;
        }
        memcpy(out + offset, header_line.data(), header_line.size());
        offset += header_line.size();

        // 检查是否是Content-Length头（大小写不敏感）
        if (!has_content_length &&
            StrCaseCmp(header.first.c_str(), "Content-Length") == 0) {
            has_content_length = true;
        }
    }

    // 3. 如果没有Content-Length且有body，自动添加
    if (!has_content_length && !resp.body.empty()) {
        std::string cl_header = "Content-Length: " + std::to_string(resp.body.size()) + "\r\n";
        if (offset + cl_header.size() > max_len) {
            set_error(PROTOCOL_ERROR_BUFFER, "Buffer too small");
            return PROTOCOL_ERROR_BUFFER;
        }
        memcpy(out + offset, cl_header.data(), cl_header.size());
        offset += cl_header.size();
    }

    // 4. 写入空行（头部结束）
    if (offset + 2 > max_len) {
        set_error(PROTOCOL_ERROR_BUFFER, "Buffer too small");
        return PROTOCOL_ERROR_BUFFER;
    }
    out[offset++] = '\r';
    out[offset++] = '\n';

    // 5. 写入Body
    if (!resp.body.empty()) {
        if (offset + resp.body.size() > max_len) {
            set_error(PROTOCOL_ERROR_BUFFER, "Buffer too small");
            return PROTOCOL_ERROR_BUFFER;
        }
        memcpy(out + offset, resp.body.data(), resp.body.size());
        offset += resp.body.size();
    }

    *out_len = offset;
    return PROTOCOL_OK;
}

void HttpParser::set_error(int code, const std::string& msg) {
    error_code_ = code;
    error_msg_ = msg;
}

int HttpParser::get_error_code() const {
    return error_code_;
}

const std::string& HttpParser::get_error_msg() const {
    return error_msg_;
}

void HttpParser::reset() {
    // 保留buffer_指针，只重置解析状态
    error_code_ = 0;
    error_msg_.clear();
}

} // namespace protocol
} // namespace https_server_sim

// 文件结束
