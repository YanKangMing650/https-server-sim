#include "connection/connection.hpp"
#include "utils/logger.hpp"
#include <unistd.h>
#include <algorithm>

namespace https_server_sim {
namespace connection {

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
    , read_buffer_(std::make_unique<utils::Buffer>())
    , write_buffer_(std::make_unique<utils::Buffer>())
    , protocol_handler_(nullptr)
    , last_activity_time_(0)
    , callback_start_time_(0)
    , in_callback_(false)
    , time_source_(time_source ? time_source : &DefaultTimeSource::instance())
    , state_callback_(nullptr)
{
    client_info_.connection_id = id;
    client_info_.server_port = server_port;
    last_activity_time_ = time_source_->get_current_time_ms();
}

Connection::~Connection() {
    // Buffer和ProtocolHandler由unique_ptr自动释放
    // 确保fd已关闭
    if (fd_ >= 0) {
        ::close(fd_);
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
    // Debug模式：校验转换规则
    if (!is_valid_state_transition(old_state, new_state)) {
        LOG_WARN("Connection", "Invalid state transition: %d -> %d (conn_id=%lu)",
                 static_cast<int>(old_state), static_cast<int>(new_state),
                 connection_id_);
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

bool Connection::is_timeout(uint32_t timeout_ms) const {
    if (state_ == ConnectionState::DISCONNECTED) {
        return false;
    }
    uint64_t current_time = time_source_->get_current_time_ms();
    return (current_time - last_activity_time_) > timeout_ms;
}

void Connection::set_callback_start_time() {
    callback_start_time_ = time_source_->get_current_time_ms();
    in_callback_ = true;
}

bool Connection::is_callback_timeout(uint32_t timeout_ms) const {
    if (!in_callback_) {
        return false;
    }
    uint64_t current_time = time_source_->get_current_time_ms();
    return (current_time - callback_start_time_) > timeout_ms;
}

void Connection::on_callback_complete() {
    in_callback_ = false;
}

utils::Buffer& Connection::get_read_buffer() {
    return *read_buffer_;
}

utils::Buffer& Connection::get_write_buffer() {
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
        // 假设ProtocolHandler有close方法
        // protocol_handler_->close();
    }

    // 关闭fd
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }

    transition_to(ConnectionState::DISCONNECTED);
}

bool Connection::is_valid_state_transition(ConnectionState from, ConnectionState to) const {
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
    }
    return false;
}

// ConnectionManager
ConnectionManager::ConnectionManager()
    : next_connection_id_(1)
    , time_source_(std::make_unique<DefaultTimeSource>())
{
}

ConnectionManager::ConnectionManager(std::unique_ptr<TimeSource> time_source)
    : next_connection_id_(1)
    , time_source_(std::move(time_source))
{
}

ConnectionManager::~ConnectionManager() {
    clear_all();
}

Connection* ConnectionManager::create_connection(int fd, uint16_t server_port) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t id = next_connection_id_++;
    auto conn = std::make_unique<Connection>(id, fd, server_port, time_source_.get());
    Connection* ptr = conn.get();
    connections_[id] = std::move(conn);
    return ptr;
}

Connection* ConnectionManager::get_connection(uint64_t conn_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = connections_.find(conn_id);
    return (it != connections_.end()) ? it->second.get() : nullptr;
}

const Connection* ConnectionManager::get_connection(uint64_t conn_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = connections_.find(conn_id);
    return (it != connections_.end()) ? it->second.get() : nullptr;
}

void ConnectionManager::remove_connection(uint64_t conn_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    connections_.erase(conn_id);
}

uint32_t ConnectionManager::get_connection_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<uint32_t>(connections_.size());
}

void ConnectionManager::for_each_connection(std::function<void(Connection&)> func) {
    std::vector<Connection*> conns;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        conns.reserve(connections_.size());
        for (auto& pair : connections_) {
            conns.push_back(pair.second.get());
        }
    }
    for (Connection* conn : conns) {
        func(*conn);
    }
}

void ConnectionManager::check_timeouts(uint32_t idle_timeout_ms,
                                        uint32_t callback_timeout_ms,
                                        std::function<void(Connection&)> on_timeout) {
    std::vector<Connection*> timeout_conns;

    // 步骤1：锁内仅收集超时连接
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& pair : connections_) {
            Connection* conn = pair.second.get();
            bool is_idle_timeout = conn->is_timeout(idle_timeout_ms);
            bool is_cb_timeout = conn->is_callback_timeout(callback_timeout_ms);
            if (is_idle_timeout || is_cb_timeout) {
                timeout_conns.push_back(conn);
            }
        }
    }

    // 步骤2：锁外调用用户回调
    for (Connection* conn : timeout_conns) {
        on_timeout(*conn);
    }
}

void ConnectionManager::clear_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    connections_.clear();
}

} // namespace connection
} // namespace https_server_sim
