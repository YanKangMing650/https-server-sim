#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <functional>
#include "utils/buffer.hpp"
#include "utils/time.hpp"
#include "protocol/protocol_handler.hpp"

namespace https_server_sim {
namespace connection {

// 前置声明
class Connection;
class ConnectionManager;

// 使用 protocol 命名空间中的 ProtocolHandler
using ProtocolHandler = protocol::ProtocolHandler;

// 连接状态枚举
enum class ConnectionState : uint8_t {
    ACCEPTING = 1,
    TLS_HANDSHAKING = 2,
    CONNECTED = 3,
    RECEIVING = 4,
    PROCESSING = 5,
    SENDING = 6,
    DISCONNECTING = 7,
    DISCONNECTED = 8
};

// Client信息结构体
struct ClientInfo {
    uint64_t connection_id;
    std::string client_ip;
    uint16_t client_port;
    uint16_t server_port;

    ClientInfo()
        : connection_id(0)
        , client_port(0)
        , server_port(0)
    {}
};

// 时间提供者接口（用于可测试性）
class TimeSource {
public:
    virtual ~TimeSource() = default;
    virtual uint64_t get_current_time_ms() const = 0;
};

// 默认时间提供者（使用系统时间）
class DefaultTimeSource : public TimeSource {
public:
    DefaultTimeSource() = default;
    ~DefaultTimeSource() = default;
    static DefaultTimeSource& instance();
    uint64_t get_current_time_ms() const override;

private:
    DefaultTimeSource(const DefaultTimeSource&) = delete;
    DefaultTimeSource& operator=(const DefaultTimeSource&) = delete;
};

// 连接状态变化回调接口
class ConnectionCallback {
public:
    virtual ~ConnectionCallback() = default;
    virtual void on_state_change(Connection& conn,
                                   ConnectionState old_state,
                                   ConnectionState new_state) = 0;
};

// 连接类
class Connection {
public:
    // 构造函数
    // id: 连接ID
    // fd: socket文件描述符
    // server_port: server监听端口
    // time_source: 时间提供者（不持有所有权）
    Connection(uint64_t id, int fd, uint16_t server_port,
               const TimeSource* time_source = nullptr);

    // 析构函数
    ~Connection();

    // 禁止拷贝
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    // ========== 基本属性 ==========

    // 获取连接ID
    uint64_t get_id() const;

    // 获取当前状态
    ConnectionState get_state() const;

    // 状态转换
    void transition_to(ConnectionState new_state);

    // 获取socket fd
    int get_fd() const;

    // ========== Client信息 ==========

    // 获取Client IP
    const std::string& get_client_ip() const;

    // 获取Client端口
    uint16_t get_client_port() const;

    // 获取Server端口
    uint16_t get_server_port() const;

    // 获取完整Client信息
    const ClientInfo& get_client_info() const;

    // 设置Client信息（connection_id和server_port已在构造函数中设置）
    void set_client_info(const std::string& ip, uint16_t port);

    // ========== 时间与超时 ==========

    // 更新最后活动时间
    void update_last_activity();

    // 检查是否空闲超时
    bool is_timeout(uint32_t timeout_ms) const;

    // 记录回调开始时间
    void set_callback_start_time();

    // 检查回调是否超时
    bool is_callback_timeout(uint32_t timeout_ms) const;

    // 回调完成
    void on_callback_complete();

    // ========== 缓冲区 ==========

    // 获取读缓冲区
    utils::Buffer& get_read_buffer();

    // 获取写缓冲区
    utils::Buffer& get_write_buffer();

    // ========== ProtocolHandler ==========

    // 设置ProtocolHandler（转移所有权）
    void set_protocol_handler(std::unique_ptr<ProtocolHandler> handler);

    // 获取ProtocolHandler指针
    ProtocolHandler* get_protocol_handler();
    const ProtocolHandler* get_protocol_handler() const;

    // ========== 回调 ==========

    // 设置状态变化回调（不持有所有权）
    void set_state_callback(ConnectionCallback* callback);

    // ========== 连接管理 ==========

    // 关闭连接
    void close();

private:
    // 检查状态转换是否合法
    bool is_valid_state_transition(ConnectionState from, ConnectionState to) const;

    uint64_t connection_id_;
    int fd_;
    ConnectionState state_;
    ClientInfo client_info_;
    std::unique_ptr<utils::Buffer> read_buffer_;
    std::unique_ptr<utils::Buffer> write_buffer_;
    std::unique_ptr<ProtocolHandler> protocol_handler_;
    uint64_t last_activity_time_;
    uint64_t callback_start_time_;
    bool in_callback_;
    const TimeSource* time_source_;
    ConnectionCallback* state_callback_;
};

// 连接管理器
class ConnectionManager {
public:
    ConnectionManager();
    explicit ConnectionManager(std::unique_ptr<TimeSource> time_source);
    ~ConnectionManager();

    // 禁止拷贝
    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;

    // 创建连接
    // fd: socket文件描述符
    // server_port: server监听端口
    // return: Connection指针（有效期至remove_connection调用前）
    Connection* create_connection(int fd, uint16_t server_port);

    // 获取连接
    Connection* get_connection(uint64_t conn_id);
    const Connection* get_connection(uint64_t conn_id) const;

    // 移除连接
    void remove_connection(uint64_t conn_id);

    // 获取连接数
    uint32_t get_connection_count() const;

    // 遍历所有连接
    void for_each_connection(std::function<void(Connection&)> func);

    // 检查超时（锁内收集，锁外回调）
    void check_timeouts(uint32_t idle_timeout_ms,
                        uint32_t callback_timeout_ms,
                        std::function<void(Connection&)> on_timeout);

    // 清空所有连接
    void clear_all();

private:
    uint64_t next_connection_id_;
    std::unordered_map<uint64_t, std::unique_ptr<Connection>> connections_;
    mutable std::mutex mutex_;
    std::unique_ptr<TimeSource> time_source_;
};

} // namespace connection
} // namespace https_server_sim
