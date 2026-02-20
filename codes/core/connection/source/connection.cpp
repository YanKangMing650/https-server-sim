// =============================================================================
//  HTTPS Server Simulator - Connection Module
//  文件: connection.cpp
//  描述: Connection 模块实现
//  版权: Copyright (c) 2026
// =============================================================================
#include "connection/connection.hpp"
#include "utils/logger.hpp"

// 平台相关头文件
#ifdef _WIN32
#include <io.h>
#define close _close
#else
#include <unistd.h>
#endif

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstdio>

namespace https_server_sim {

// ConnectionState 字符串化函数
const char* connection_state_to_string(ConnectionState state) {
    switch (state) {
        case ConnectionState::ACCEPTING:
            return "ACCEPTING";
        case ConnectionState::TLS_HANDSHAKING:
            return "TLS_HANDSHAKING";
        case ConnectionState::CONNECTED:
            return "CONNECTED";
        case ConnectionState::RECEIVING:
            return "RECEIVING";
        case ConnectionState::PROCESSING:
            return "PROCESSING";
        case ConnectionState::SENDING:
            return "SENDING";
        case ConnectionState::DISCONNECTING:
            return "DISCONNECTING";
        case ConnectionState::DISCONNECTED:
            return "DISCONNECTED";
        default:
            return "UNKNOWN";
    }
}

// DefaultTimeSource
DefaultTimeSource& DefaultTimeSource::instance() {
    static DefaultTimeSource instance;
    return instance;
}

uint64_t DefaultTimeSource::get_current_time_ms() const {
    return utils::get_current_time_ms();
}

// Connection
Connection::Connection(uint64_t id, int fd, uint16_t server_port,
                       const TimeSource* time_source)
    : connection_id_(id)
    , fd_(fd)
    , state_(ConnectionState::ACCEPTING)
    , client_info_()
    , read_buffer_(std::make_unique<utils::Buffer>())
    , write_buffer_(std::make_unique<utils::Buffer>())
    , protocol_handler_(nullptr)
    , last_activity_time_(time_source ? time_source->get_current_time_ms() : DefaultTimeSource::instance().get_current_time_ms())
    , callback_start_time_(0)
    , in_callback_(false)
    , time_source_(time_source ? time_source : &DefaultTimeSource::instance())
    , state_callback_(nullptr)
{
    client_info_.connection_id = id;
    client_info_.server_port = server_port;
}

Connection::~Connection() {
    // Buffer和ProtocolHandler由unique_ptr自动释放
    // 确保fd已关闭
    close_fd();
}

void Connection::close_fd() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

uint64_t Connection::get_id() const {
    return connection_id_;
}

ConnectionState Connection::get_state() const {
    return state_;
}

void Connection::transition_to(ConnectionState new_state) {
    if (state_ == new_state) {
        return;
    }
    if (state_ == ConnectionState::DISCONNECTED) {
        return;
    }

    ConnectionState old_state = state_;

#ifndef NDEBUG
    // Debug模式：校验转换规则，非法转换记录警告日志并assert终止
    bool valid_transition = is_valid_state_transition(old_state, new_state);
    if (!valid_transition) {
        char msg[128];
        std::snprintf(msg, sizeof(msg),
                     "Invalid state transition: %d -> %d (conn_id=%" PRIu64 ")",
                     static_cast<int>(old_state), static_cast<int>(new_state),
                     connection_id_);
        LOG_WARN("Connection", "%s", msg);
        assert(false && "Invalid state transition in Debug mode");
    }
#endif

    // 执行状态转换
    state_ = new_state;
    update_last_activity();

    // 触发状态变化回调
    if (state_callback_ != nullptr) {
        state_callback_->on_state_change(*this, old_state, new_state);
    }

    // 特定状态转换的附加逻辑
    if (new_state == ConnectionState::DISCONNECTED) {
        in_callback_ = false;
    }
}

int Connection::get_fd() const {
    return fd_;
}

bool Connection::is_fd_valid() const {
    return fd_ >= 0 && state_ != ConnectionState::DISCONNECTED;
}

const std::string& Connection::get_client_ip() const {
    return client_info_.client_ip;
}

uint16_t Connection::get_client_port() const {
    return client_info_.client_port;
}

uint16_t Connection::get_server_port() const {
    return client_info_.server_port;
}

const ClientInfo& Connection::get_client_info() const {
    return client_info_;
}

void Connection::set_client_info(const std::string& ip, uint16_t port) {
    client_info_.client_ip = ip;
    client_info_.client_port = port;
}

void Connection::update_last_activity() {
    last_activity_time_ = time_source_->get_current_time_ms();
}

// 检查是否空闲超时
// 参数: timeout_ms - 超时时间（毫秒），若为0表示永不超时
// 返回: true表示超时，false表示未超时；DISCONNECTED状态始终返回false
bool Connection::is_timeout(uint32_t timeout_ms) const {
    if (state_ == ConnectionState::DISCONNECTED) {
        return false;
    }
    if (timeout_ms == 0) {
        // timeout_ms为0时，表示永不超时
        return false;
    }
    uint64_t current_time = time_source_->get_current_time_ms();
    return (current_time - last_activity_time_) > timeout_ms;
}

void Connection::set_callback_start_time() {
    callback_start_time_ = time_source_->get_current_time_ms();
    in_callback_ = true;
}

// 检查回调是否超时
// 参数: timeout_ms - 超时时间（毫秒），若为0表示永不超时
// 返回: true表示超时，false表示未超时；不在回调中时始终返回false
bool Connection::is_callback_timeout(uint32_t timeout_ms) const {
    if (!in_callback_) {
        return false;
    }
    if (timeout_ms == 0) {
        // timeout_ms为0时，表示永不超时
        return false;
    }
    uint64_t current_time = time_source_->get_current_time_ms();
    return (current_time - callback_start_time_) > timeout_ms;
}

void Connection::on_callback_complete() {
    in_callback_ = false;
}

utils::Buffer& Connection::get_read_buffer() {
    assert(read_buffer_ != nullptr && "read_buffer_ should not be nullptr");
    return *read_buffer_;
}

utils::Buffer& Connection::get_write_buffer() {
    assert(write_buffer_ != nullptr && "write_buffer_ should not be nullptr");
    return *write_buffer_;
}

void Connection::set_protocol_handler(std::unique_ptr<ProtocolHandler> handler) {
    protocol_handler_ = std::move(handler);
}

ProtocolHandler* Connection::get_protocol_handler() {
    return protocol_handler_.get();
}

const ProtocolHandler* Connection::get_protocol_handler() const {
    return protocol_handler_.get();
}

void Connection::set_state_callback(ConnectionCallback* callback) {
    state_callback_ = callback;
}

void Connection::close() {
    if (state_ == ConnectionState::DISCONNECTED) {
        return;
    }

    transition_to(ConnectionState::DISCONNECTING);

    // 通知ProtocolHandler关闭
    if (protocol_handler_) {
        protocol_handler_->close();
    }

    // 关闭fd
    close_fd();

    transition_to(ConnectionState::DISCONNECTED);
}

bool Connection::is_valid_state_transition(ConnectionState from, ConnectionState to) const {
    // 按照设计文档的状态转换矩阵实现：保持当前状态始终合法
    switch (from) {
        case ConnectionState::ACCEPTING:
            return to == ConnectionState::ACCEPTING ||
                   to == ConnectionState::TLS_HANDSHAKING ||
                   to == ConnectionState::DISCONNECTING;
        case ConnectionState::TLS_HANDSHAKING:
            return to == ConnectionState::TLS_HANDSHAKING ||
                   to == ConnectionState::CONNECTED ||
                   to == ConnectionState::DISCONNECTING;
        case ConnectionState::CONNECTED:
            return to == ConnectionState::CONNECTED ||
                   to == ConnectionState::RECEIVING ||
                   to == ConnectionState::DISCONNECTING;
        case ConnectionState::RECEIVING:
            return to == ConnectionState::RECEIVING ||
                   to == ConnectionState::PROCESSING ||
                   to == ConnectionState::DISCONNECTING;
        case ConnectionState::PROCESSING:
            return to == ConnectionState::PROCESSING ||
                   to == ConnectionState::SENDING ||
                   to == ConnectionState::DISCONNECTING;
        case ConnectionState::SENDING:
            return to == ConnectionState::SENDING ||
                   to == ConnectionState::CONNECTED ||
                   to == ConnectionState::DISCONNECTING;
        case ConnectionState::DISCONNECTING:
            return to == ConnectionState::DISCONNECTING ||
                   to == ConnectionState::DISCONNECTED;
        case ConnectionState::DISCONNECTED:
            return to == ConnectionState::DISCONNECTED;
        default:
            return false;
    }
}

} // namespace https_server_sim

// 文件结束
