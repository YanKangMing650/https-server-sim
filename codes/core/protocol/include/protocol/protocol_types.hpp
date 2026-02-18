// =============================================================================
//  HTTPS Server Simulator - Protocol Module
//  文件: protocol_types.hpp
//  描述: Protocol模块类型定义、枚举、常量
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <map>
#include <vector>

namespace https_server_sim {
namespace protocol {

// ==================== 错误码定义 ====================
constexpr int PROTOCOL_OK = 0;
constexpr int PROTOCOL_ERROR_EAGAIN = -EAGAIN;  // 按设计文档使用-EAGAIN
constexpr int PROTOCOL_ERROR_INVALID = -1;
constexpr int PROTOCOL_ERROR_TOO_LONG = -2;
constexpr int PROTOCOL_ERROR_TOO_MANY = -3;
constexpr int PROTOCOL_ERROR_BUFFER = -4;
constexpr int PROTOCOL_ERROR_VERSION = -5;
constexpr int PROTOCOL_ERROR_TLS = -10;

// ==================== HTTP解析相关常量 ====================
constexpr size_t MAX_LINE_LEN = 8192;
constexpr size_t MAX_HEADER_LINE_LEN = 8192;
constexpr size_t MAX_HEADERS = 100;
constexpr size_t MAX_HEADER_NAME_LEN = 256;
constexpr size_t MAX_HEADER_VALUE_LEN = 4096;
constexpr size_t MAX_BODY_SIZE = 64 * 1024 * 1024;

// ==================== HTTP/2相关常量 ====================
constexpr size_t HTTP2_FRAME_HEADER_SIZE = 9;
constexpr uint32_t HTTP2_DEFAULT_MAX_FRAME_SIZE = 16384;
constexpr uint32_t HTTP2_DEFAULT_INITIAL_WINDOW_SIZE = 65535;

// ==================== 通用缓冲区常量 ====================
constexpr size_t TEMP_BUFFER_SIZE = 4096;

// ==================== HTTP/2帧标志 ====================
constexpr uint8_t HTTP2_FLAG_END_STREAM = 0x01;
constexpr uint8_t HTTP2_FLAG_END_HEADERS = 0x04;
constexpr uint8_t HTTP2_FLAG_PADDED = 0x08;
constexpr uint8_t HTTP2_FLAG_PRIORITY = 0x20;
constexpr uint8_t HTTP2_FLAG_ACK = 0x01;

// ==================== 协议类型枚举 ====================
enum class ProtocolType {
    UNKNOWN = 2,
    HTTP_1_1 = 0,
    HTTP_2 = 1
};

// ==================== HTTP/1.1解析状态枚举 ====================
enum class Http1ParseState {
    EXPECT_REQUEST_LINE = 0,
    EXPECT_HEADERS = 1,
    EXPECT_BODY = 2,
    EXPECT_COMPLETE = 3,
    ERROR = 4
};

// ==================== HTTP/2流状态枚举 ====================
enum class Http2StreamState {
    IDLE = 0,
    OPEN = 1,
    HALF_CLOSED_LOCAL = 2,
    HALF_CLOSED_REMOTE = 3,
    CLOSED = 4
};

// ==================== HTTP/2帧类型枚举 ====================
enum class Http2FrameType : uint8_t {
    DATA = 0x0,
    HEADERS = 0x1,
    PRIORITY = 0x2,
    RST_STREAM = 0x3,
    SETTINGS = 0x4,
    PUSH_PROMISE = 0x5,
    PING = 0x6,
    GOAWAY = 0x7,
    WINDOW_UPDATE = 0x8,
    CONTINUATION = 0x9
};

// ==================== 证书类型枚举 ====================
enum class CertType {
    RSA = 0,
    ECDSA = 1,
    SM2 = 2
};

// ==================== 解析结果枚举 ====================
enum class ParseResult {
    OK = 0,
    NEED_MORE = 1,
    ERROR = 2
};

// ==================== HTTP头部集合类型 ====================
using HttpHeaders = std::map<std::string, std::string>;

// ==================== 证书配置结构体 ====================
struct CertConfig {
    std::string cert_path;
    std::string key_path;
    std::string ca_path;
    bool use_gmssl;

    CertConfig()
        : use_gmssl(false)
    {
    }
};

// ==================== TLS配置结构体 ====================
struct TlsConfig {
    std::string cipher_suites;
    std::string alpn_protocols;
    bool enable_tls_1_3;
    bool enable_tls_1_2;

    TlsConfig()
        : enable_tls_1_3(true)
        , enable_tls_1_2(true)
    {
    }
};

// ==================== HTTP/2帧头结构体 ====================
struct Http2FrameHeader {
    uint32_t length;
    Http2FrameType type;
    uint8_t flags;
    uint32_t stream_id;

    static constexpr size_t SIZE = 9;

    Http2FrameHeader()
        : length(0)
        , type(Http2FrameType::DATA)
        , flags(0)
        , stream_id(0)
    {
    }
};

// ==================== HTTP/2 SETTINGS结构体 ====================
struct Http2Settings {
    uint32_t header_table_size;
    uint32_t enable_push;
    uint32_t max_concurrent_streams;
    uint32_t max_frame_size;
    uint32_t initial_window_size;
    uint32_t max_header_list_size;

    void reset() {
        header_table_size = 4096;
        enable_push = 0;
        max_concurrent_streams = 100;
        initial_window_size = 65535;
        max_frame_size = 16384;
        max_header_list_size = 0xFFFFFFFF;
    }

    Http2Settings() {
        reset();
    }
};

} // namespace protocol
} // namespace https_server_sim

// 文件结束
