// =============================================================================
//  HTTPS Server Simulator - Utils Module
//  文件: error.hpp
//  描述: 统一错误码定义
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include <cstdint>
#include <string>

namespace https_server_sim {
namespace utils {

// 统一错误码定义
enum class ErrorCode : int32_t {
    // 通用错误 (0-999)
    SUCCESS = 0,
    UNKNOWN_ERROR = 1,
    INVALID_ARGUMENT = 2,
    NULL_POINTER = 3,
    OUT_OF_MEMORY = 4,
    OUT_OF_RANGE = 5,
    NOT_INITIALIZED = 6,
    ALREADY_INITIALIZED = 7,
    OPERATION_FAILED = 8,
    TIMEOUT = 9,
    BUFFER_OVERFLOW = 10,
    BUFFER_UNDERFLOW = 11,

    // 文件IO错误 (1000-1999)
    FILE_NOT_FOUND = 1000,
    FILE_PERMISSION_DENIED = 1001,
    FILE_READ_ERROR = 1002,
    FILE_WRITE_ERROR = 1003,
    FILE_CLOSE_ERROR = 1004,

    // 配置错误 (2000-2999)
    CONFIG_PARSE_ERROR = 2000,
    CONFIG_MISSING_REQUIRED = 2001,
    CONFIG_INVALID_VALUE = 2002,
    CONFIG_INVALID_PORT = 2003,
    CONFIG_INVALID_THREAD_COUNT = 2004,
    CONFIG_INVALID_LOG_LEVEL = 2005,

    // 网络错误 (3000-3999)
    NETWORK_SOCKET_ERROR = 3000,
    NETWORK_BIND_ERROR = 3001,
    NETWORK_LISTEN_ERROR = 3002,
    NETWORK_ACCEPT_ERROR = 3003,
    NETWORK_CONNECT_ERROR = 3004,
    NETWORK_READ_ERROR = 3005,
    NETWORK_WRITE_ERROR = 3006,
    NETWORK_CLOSED = 3007,

    // TLS错误 (4000-4999)
    TLS_INIT_ERROR = 4000,
    TLS_CERT_ERROR = 4001,
    TLS_KEY_ERROR = 4002,
    TLS_HANDSHAKE_ERROR = 4003,
    TLS_ENCRYPT_ERROR = 4004,
    TLS_DECRYPT_ERROR = 4005,

    // 协议错误 (5000-5999)
    PROTOCOL_INVALID_FRAME = 5000,
    PROTOCOL_INVALID_HEADER = 5001,
    PROTOCOL_FRAME_TOO_LARGE = 5002,
    PROTOCOL_STREAM_ERROR = 5003,
    PROTOCOL_GOAWAY = 5004,

    // Buffer错误 (6000-6999)
    BUFFER_TOO_SMALL = 6000,
    BUFFER_CAPACITY_EXCEEDED = 6001,
    BUFFER_NO_DATA = 6002,
};

// 错误码转字符串
const char* error_code_to_string(ErrorCode code);

// 错误码转描述
const char* error_code_to_description(ErrorCode code);

// 判断是否成功
inline bool is_success(ErrorCode code) {
    return code == ErrorCode::SUCCESS;
}

// 判断是否失败
inline bool is_error(ErrorCode code) {
    return code != ErrorCode::SUCCESS;
}

// 错误结果类（带错误码的返回值包装）
template<typename T>
class Result {
public:
    // 成功构造
    explicit Result(const T& value)
        : code_(ErrorCode::SUCCESS)
        , value_(value)
        , has_value_(true)
    {}

    explicit Result(T&& value)
        : code_(ErrorCode::SUCCESS)
        , value_(std::move(value))
        , has_value_(true)
    {}

    // Emplace风格构造（直接构造value，避免拷贝）
    template<typename... Args>
    static Result ok(Args&&... args) {
        Result result;
        result.code_ = ErrorCode::SUCCESS;
        new (&result.value_) T(std::forward<Args>(args)...);
        result.has_value_ = true;
        return result;
    }

    // 失败构造
    explicit Result(ErrorCode code)
        : code_(code)
        , message_(error_code_to_description(code))
        , value_()
        , has_value_(false)
    {}

    Result(ErrorCode code, const std::string& message)
        : code_(code)
        , message_(message)
        , value_()
        , has_value_(false)
    {}

    Result(ErrorCode code, std::string&& message)
        : code_(code)
        , message_(std::move(message))
        , value_()
        , has_value_(false)
    {}

    // 是否成功 - 两套命名风格都支持
    bool is_ok() const { return has_value_; }
    bool is_err() const { return !has_value_; }
    bool is_success() const { return has_value_; }
    bool is_error() const { return !has_value_; }

    // 获取值（必须确保成功）
    const T& value() const { return value_; }
    T& value() { return value_; }

    // 获取值，带默认值
    const T& value_or(const T& default_value) const {
        return has_value_ ? value_ : default_value;
    }

    // 获取错误码
    ErrorCode error_code() const { return code_; }

    // 获取错误消息
    const std::string& error_message() const {
        return message_;
    }

private:
    ErrorCode code_;
    std::string message_;
    T value_;
    bool has_value_;
};

// 特化void版本
template<>
class Result<void> {
public:
    Result() : code_(ErrorCode::SUCCESS) {}

    explicit Result(ErrorCode code)
        : code_(code)
        , message_(error_code_to_description(code))
    {}

    Result(ErrorCode code, const std::string& message)
        : code_(code)
        , message_(message)
    {}

    Result(ErrorCode code, std::string&& message)
        : code_(code)
        , message_(std::move(message))
    {}

    // 是否成功 - 两套命名风格都支持
    bool is_ok() const { return code_ == ErrorCode::SUCCESS; }
    bool is_err() const { return code_ != ErrorCode::SUCCESS; }
    bool is_success() const { return code_ == ErrorCode::SUCCESS; }
    bool is_error() const { return code_ != ErrorCode::SUCCESS; }

    ErrorCode error_code() const { return code_; }

    const std::string& error_message() const {
        return message_;
    }

private:
    ErrorCode code_;
    std::string message_;
};

// 辅助函数创建成功结果
template<typename T>
Result<T> make_ok(const T& value) {
    return Result<T>(value);
}

template<typename T>
Result<T> make_ok(T&& value) {
    return Result<T>(std::forward<T>(value));
}

inline Result<void> make_ok() {
    return Result<void>();
}

// 辅助函数创建错误结果
template<typename T>
Result<T> make_err(ErrorCode code) {
    return Result<T>(code);
}

template<typename T>
Result<T> make_err(ErrorCode code, const std::string& message) {
    return Result<T>(code, message);
}

template<typename T>
Result<T> make_err(ErrorCode code, std::string&& message) {
    return Result<T>(code, std::move(message));
}

inline Result<void> make_err(ErrorCode code) {
    return Result<void>(code);
}

inline Result<void> make_err(ErrorCode code, const std::string& message) {
    return Result<void>(code, message);
}

inline Result<void> make_err(ErrorCode code, std::string&& message) {
    return Result<void>(code, std::move(message));
}

} // namespace utils
} // namespace https_server_sim
