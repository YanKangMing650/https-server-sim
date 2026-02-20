// =============================================================================
//  HTTPS Server Simulator - Integration Test
//  文件: test_connection_manager.cpp
//  描述: 连接管理集成测试 - 验证Server + ConnectionManager + Connection模块协作
//  版权: Copyright (c) 2026
// =============================================================================

#include <gtest/gtest.h>
#include <fstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include "server/server.hpp"
#include "connection/connection_manager.hpp"
#include "connection/connection.hpp"
#include "utils/buffer.hpp"

namespace https_server_sim {
namespace integration_test {

// ==================== 测试辅助类 ====================

class MockTimeSource : public TimeSource {
public:
    MockTimeSource() : current_time_ms_(0) {}
    ~MockTimeSource() override = default;

    uint64_t get_current_time_ms() const override {
        return current_time_ms_;
    }

    void set_time(uint64_t time_ms) {
        current_time_ms_ = time_ms;
    }

    void advance_time(uint64_t delta_ms) {
        current_time_ms_ += delta_ms;
    }

private:
    uint64_t current_time_ms_;
};

// ==================== 集成测试用例 ====================

// Integration_UseCase006: ConnectionManager创建和获取Connection
// 验证: ConnectionManager能创建Connection并通过ID找回
TEST(IntegrationTest, UseCase006_CreateAndGetConnection) {
    ConnectionManager mgr;

    // 创建Connection
    int fd = 100;  // 模拟fd
    uint16_t port = 8443;

    auto conn = mgr.create_connection(fd, port);
    ASSERT_TRUE(conn != nullptr) << "create_connection should return non-null";

    // 验证Connection属性
    EXPECT_EQ(conn->get_fd(), fd);
    EXPECT_EQ(conn->get_server_port(), port);
    EXPECT_EQ(conn->get_state(), ConnectionState::ACCEPTING);

    // 通过ID找回Connection
    uint64_t conn_id = conn->get_id();
    auto retrieved = mgr.get_connection(conn_id);
    ASSERT_TRUE(retrieved != nullptr) << "get_connection should find the connection";
    EXPECT_EQ(retrieved->get_id(), conn_id);
}

// Integration_UseCase007: ConnectionManager移除Connection
// 验证: 移除后不能再通过ID找到
TEST(IntegrationTest, UseCase007_RemoveConnection) {
    ConnectionManager mgr;

    // 创建Connection
    auto conn = mgr.create_connection(100, 8443);
    uint64_t conn_id = conn->get_id();

    // 验证存在
    EXPECT_EQ(mgr.get_connection_count(), 1U);

    // 移除
    mgr.remove_connection(conn_id);

    // 验证不存在
    EXPECT_EQ(mgr.get_connection_count(), 0U);
    auto retrieved = mgr.get_connection(conn_id);
    EXPECT_TRUE(retrieved == nullptr);
}

// Integration_UseCase008: Connection状态转换
// 验证: Connection能正确转换状态
TEST(IntegrationTest, UseCase008_ConnectionStateTransitions) {
    MockTimeSource time_source;
    ConnectionManager mgr(std::make_unique<MockTimeSource>(time_source));

    auto conn = mgr.create_connection(100, 8443);

    // 初始状态: ACCEPTING
    EXPECT_EQ(conn->get_state(), ConnectionState::ACCEPTING);

    // 转换到TLS_HANDSHAKING
    conn->transition_to(ConnectionState::TLS_HANDSHAKING);
    EXPECT_EQ(conn->get_state(), ConnectionState::TLS_HANDSHAKING);

    // 转换到CONNECTED
    conn->transition_to(ConnectionState::CONNECTED);
    EXPECT_EQ(conn->get_state(), ConnectionState::CONNECTED);

    // 转换到RECEIVING
    conn->transition_to(ConnectionState::RECEIVING);
    EXPECT_EQ(conn->get_state(), ConnectionState::RECEIVING);

    // 转换到PROCESSING
    conn->transition_to(ConnectionState::PROCESSING);
    EXPECT_EQ(conn->get_state(), ConnectionState::PROCESSING);

    // 转换到SENDING
    conn->transition_to(ConnectionState::SENDING);
    EXPECT_EQ(conn->get_state(), ConnectionState::SENDING);

    // 转换回CONNECTED
    conn->transition_to(ConnectionState::CONNECTED);
    EXPECT_EQ(conn->get_state(), ConnectionState::CONNECTED);

    // 转换到DISCONNECTING
    conn->transition_to(ConnectionState::DISCONNECTING);
    EXPECT_EQ(conn->get_state(), ConnectionState::DISCONNECTING);

    // 转换到DISCONNECTED
    conn->transition_to(ConnectionState::DISCONNECTED);
    EXPECT_EQ(conn->get_state(), ConnectionState::DISCONNECTED);
}

// Integration_UseCase009: Connection超时检测
// 验证: ConnectionManager能检测空闲超时
TEST(IntegrationTest, UseCase009_ConnectionTimeoutDetection) {
    MockTimeSource time_source;
    ConnectionManager mgr(std::make_unique<MockTimeSource>(time_source));

    // 创建Connection
    auto conn = mgr.create_connection(100, 8443);
    uint64_t conn_id = conn->get_id();

    // 设置初始时间
    time_source.set_time(1000);
    conn->update_last_activity();

    // 超时前检查
    time_source.set_time(1000 + 29000);  // 29秒后，未超时
    EXPECT_FALSE(conn->is_timeout(30000));

    // 超时后检查
    time_source.set_time(1000 + 31000);  // 31秒后，超时
    EXPECT_TRUE(conn->is_timeout(30000));
}

// Integration_UseCase010: ConnectionManager遍历所有连接
// 验证: for_each_connection能遍历所有连接
TEST(IntegrationTest, UseCase010_ForEachConnection) {
    ConnectionManager mgr;

    // 创建多个连接
    std::vector<uint64_t> conn_ids;
    for (int i = 0; i < 5; i++) {
        auto conn = mgr.create_connection(100 + i, 8443);
        conn_ids.push_back(conn->get_id());
    }

    EXPECT_EQ(mgr.get_connection_count(), 5U);

    // 遍历
    std::atomic<int> count{0};
    mgr.for_each_connection([&count](Connection&) {
        count++;
    });

    EXPECT_EQ(count.load(), 5);
}

// Integration_UseCase011: Connection Buffer读写
// 验证: Connection的Buffer能正常读写
TEST(IntegrationTest, UseCase011_ConnectionBufferReadWrite) {
    ConnectionManager mgr;
    auto conn = mgr.create_connection(100, 8443);

    // 获取读缓冲区
    utils::Buffer& read_buf = conn->get_read_buffer();

    // 写入数据
    const uint8_t write_data[] = "Hello, Connection!";
    size_t write_len = 0;
    int ret = read_buf.write(write_data, sizeof(write_data), &write_len);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(write_len, sizeof(write_data));

    // 验证可读数据
    EXPECT_EQ(read_buf.readable_bytes(), sizeof(write_data));

    // 读取数据
    uint8_t read_buf_data[128];
    size_t read_len = 0;
    ret = read_buf.read(read_buf_data, sizeof(read_buf_data), &read_len);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(read_len, sizeof(write_data));
    EXPECT_STREQ(reinterpret_cast<const char*>(read_buf_data),
                 reinterpret_cast<const char*>(write_data));
}

// Integration_UseCase012: 多线程安全访问ConnectionManager
// 验证: ConnectionManager在多线程下安全
TEST(IntegrationTest, UseCase012_MultiThreadedConnectionManager) {
    ConnectionManager mgr;
    std::atomic<bool> error_occurred{false};

    // 线程1: 创建连接
    std::thread creator([&]() {
        try {
            for (int i = 0; i < 100; i++) {
                mgr.create_connection(200 + i, 8443);
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        } catch (...) {
            error_occurred = true;
        }
    });

    // 线程2: 遍历连接
    std::thread iterator([&]() {
        try {
            for (int i = 0; i < 50; i++) {
                mgr.for_each_connection([](Connection&) {
                    // 只是访问，不做修改
                });
                std::this_thread::sleep_for(std::chrono::microseconds(20));
            }
        } catch (...) {
            error_occurred = true;
        }
    });

    // 等待线程结束
    creator.join();
    iterator.join();

    EXPECT_FALSE(error_occurred.load()) << "No exception should occur";
    EXPECT_GE(mgr.get_connection_count(), 50U);
}

} // namespace integration_test
} // namespace https_server_sim

// 文件结束
