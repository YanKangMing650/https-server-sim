// =============================================================================
//  HTTPS Server Simulator - Connection Module Unit Tests
//  文件: test_connection.cpp
//  描述: Connection 模块单元测试
//  版权: Copyright (c) 2026
// =============================================================================
#include <gtest/gtest.h>
#include "connection/connection.hpp"
#include "connection/connection_manager.hpp"
#include "protocol/protocol_handler.hpp"

namespace https_server_sim {

// ==================== 测试常量定义 ====================

namespace {

// 测试用的文件描述符（避免使用0-2的标准流）
constexpr int TEST_FD_1 = 100;
constexpr int TEST_FD_2 = 101;
constexpr int TEST_FD_3 = 102;
constexpr int TEST_FD_4 = 103;

// 测试用的服务器端口
constexpr uint16_t TEST_PORT = 443;

} // anonymous namespace

// ==================== Mock 类定义 ====================

// Mock TimeSource 用于测试
class MockTimeSource : public TimeSource {
public:
    mutable uint64_t mock_time_ms_;

    MockTimeSource() : mock_time_ms_(0) {}

    uint64_t get_current_time_ms() const override {
        return mock_time_ms_;
    }

    void advance_time(uint64_t ms) {
        mock_time_ms_ += ms;
    }

    void set_time(uint64_t ms) {
        mock_time_ms_ = ms;
    }
};

// Mock ConnectionCallback 用于测试
class MockConnectionCallback : public ConnectionCallback {
public:
    int call_count_;
    Connection* last_conn_;
    ConnectionState last_old_state_;
    ConnectionState last_new_state_;

    MockConnectionCallback()
        : call_count_(0)
        , last_conn_(nullptr)
        , last_old_state_(ConnectionState::ACCEPTING)
        , last_new_state_(ConnectionState::ACCEPTING)
    {}

    void on_state_change(Connection& conn,
                         ConnectionState old_state,
                         ConnectionState new_state) override {
        call_count_++;
        last_conn_ = &conn;
        last_old_state_ = old_state;
        last_new_state_ = new_state;
    }
};

// Mock ProtocolHandler 用于测试
class MockProtocolHandler : public protocol::ProtocolHandler {
public:
    bool closed_;
    std::string name_;

    MockProtocolHandler() : closed_(false), name_("MockProtocol") {}

    const std::string& get_name() const override {
        return name_;
    }

    void close() override {
        closed_ = true;
    }
};

// ==================== Connection 测试 ====================

// Conn_UT_001: 初始状态
TEST(ConnectionTest, InitialStateIsAccepting) {
    MockTimeSource timeSource;
    Connection conn(1, 5, 443, &timeSource);
    EXPECT_EQ(conn.get_state(), ConnectionState::ACCEPTING);
}

// Conn_UT_002: ACCEPTING→TLS_HANDSHAKING→CONNECTED
TEST(ConnectionTest, NormalHandshakeTransition) {
    MockTimeSource timeSource;
    Connection conn(1, 5, 443, &timeSource);

    conn.transition_to(ConnectionState::TLS_HANDSHAKING);
    EXPECT_EQ(conn.get_state(), ConnectionState::TLS_HANDSHAKING);

    conn.transition_to(ConnectionState::CONNECTED);
    EXPECT_EQ(conn.get_state(), ConnectionState::CONNECTED);
}

// Conn_UT_003: CONNECTED→RECEIVING→PROCESSING→SENDING→CONNECTED
TEST(ConnectionTest, FullRequestResponseCycle) {
    MockTimeSource timeSource;
    Connection conn(1, 5, 443, &timeSource);
    conn.transition_to(ConnectionState::TLS_HANDSHAKING);
    conn.transition_to(ConnectionState::CONNECTED);

    conn.transition_to(ConnectionState::RECEIVING);
    EXPECT_EQ(conn.get_state(), ConnectionState::RECEIVING);

    conn.transition_to(ConnectionState::PROCESSING);
    EXPECT_EQ(conn.get_state(), ConnectionState::PROCESSING);

    conn.transition_to(ConnectionState::SENDING);
    EXPECT_EQ(conn.get_state(), ConnectionState::SENDING);

    conn.transition_to(ConnectionState::CONNECTED);
    EXPECT_EQ(conn.get_state(), ConnectionState::CONNECTED);
}

// Conn_UT_004: DISCONNECTED后不可再转
TEST(ConnectionTest, DisconnectedStateIsFinal) {
    MockTimeSource timeSource;
    Connection conn(1, 5, 443, &timeSource);

    conn.transition_to(ConnectionState::DISCONNECTING);
    conn.transition_to(ConnectionState::DISCONNECTED);
    EXPECT_EQ(conn.get_state(), ConnectionState::DISCONNECTED);

    conn.transition_to(ConnectionState::CONNECTED);
    EXPECT_EQ(conn.get_state(), ConnectionState::DISCONNECTED);
}

// Conn_UT_005: 非法转换检测 (测试is_valid_state_transition逻辑)
// 注意：Debug模式会assert，Release模式纯功能驱动
TEST(ConnectionTest, InvalidStateTransitionCheck) {
    MockTimeSource timeSource;
    Connection conn(1, 5, 443, &timeSource);

    // 先正常转到 CONNECTED
    conn.transition_to(ConnectionState::TLS_HANDSHAKING);
    conn.transition_to(ConnectionState::CONNECTED);
    [[maybe_unused]] auto current_state = conn.get_state();  // 仅用于验证

    // 测试仅在Release模式下进行非法转换
    // Debug模式会assert终止，因此不执行这部分
#ifdef NDEBUG
    // 尝试直接从 CONNECTED 到 PROCESSING（不合法）
    // 按照设计文档，Release模式纯功能驱动，仍会执行转换
    conn.transition_to(ConnectionState::PROCESSING);
    EXPECT_EQ(conn.get_state(), ConnectionState::PROCESSING);
#endif

    // 即使转换非法，代码仍继续执行（符合设计文档要求）
}

// 直接测试 is_valid_state_transition 方法 (CONN-001)
// 使用友元类直接访问private方法
TEST(ConnectionTest, IsValidStateTransitionDirect) {
    MockTimeSource timeSource;
    Connection conn(1, 5, 443, &timeSource);

    // ACCEPTING 的合法转换
    EXPECT_TRUE(conn.is_valid_state_transition(ConnectionState::ACCEPTING, ConnectionState::ACCEPTING));
    EXPECT_TRUE(conn.is_valid_state_transition(ConnectionState::ACCEPTING, ConnectionState::TLS_HANDSHAKING));
    EXPECT_TRUE(conn.is_valid_state_transition(ConnectionState::ACCEPTING, ConnectionState::DISCONNECTING));
    EXPECT_FALSE(conn.is_valid_state_transition(ConnectionState::ACCEPTING, ConnectionState::CONNECTED));
    EXPECT_FALSE(conn.is_valid_state_transition(ConnectionState::ACCEPTING, ConnectionState::RECEIVING));

    // TLS_HANDSHAKING 的合法转换
    EXPECT_TRUE(conn.is_valid_state_transition(ConnectionState::TLS_HANDSHAKING, ConnectionState::TLS_HANDSHAKING));
    EXPECT_TRUE(conn.is_valid_state_transition(ConnectionState::TLS_HANDSHAKING, ConnectionState::CONNECTED));
    EXPECT_TRUE(conn.is_valid_state_transition(ConnectionState::TLS_HANDSHAKING, ConnectionState::DISCONNECTING));
    EXPECT_FALSE(conn.is_valid_state_transition(ConnectionState::TLS_HANDSHAKING, ConnectionState::RECEIVING));

    // CONNECTED 的合法转换
    EXPECT_TRUE(conn.is_valid_state_transition(ConnectionState::CONNECTED, ConnectionState::CONNECTED));
    EXPECT_TRUE(conn.is_valid_state_transition(ConnectionState::CONNECTED, ConnectionState::RECEIVING));
    EXPECT_TRUE(conn.is_valid_state_transition(ConnectionState::CONNECTED, ConnectionState::DISCONNECTING));
    EXPECT_FALSE(conn.is_valid_state_transition(ConnectionState::CONNECTED, ConnectionState::PROCESSING));

    // RECEIVING 的合法转换
    EXPECT_TRUE(conn.is_valid_state_transition(ConnectionState::RECEIVING, ConnectionState::RECEIVING));
    EXPECT_TRUE(conn.is_valid_state_transition(ConnectionState::RECEIVING, ConnectionState::PROCESSING));
    EXPECT_TRUE(conn.is_valid_state_transition(ConnectionState::RECEIVING, ConnectionState::DISCONNECTING));
    EXPECT_FALSE(conn.is_valid_state_transition(ConnectionState::RECEIVING, ConnectionState::SENDING));

    // PROCESSING 的合法转换
    EXPECT_TRUE(conn.is_valid_state_transition(ConnectionState::PROCESSING, ConnectionState::PROCESSING));
    EXPECT_TRUE(conn.is_valid_state_transition(ConnectionState::PROCESSING, ConnectionState::SENDING));
    EXPECT_TRUE(conn.is_valid_state_transition(ConnectionState::PROCESSING, ConnectionState::DISCONNECTING));
    EXPECT_FALSE(conn.is_valid_state_transition(ConnectionState::PROCESSING, ConnectionState::CONNECTED));

    // SENDING 的合法转换
    EXPECT_TRUE(conn.is_valid_state_transition(ConnectionState::SENDING, ConnectionState::SENDING));
    EXPECT_TRUE(conn.is_valid_state_transition(ConnectionState::SENDING, ConnectionState::CONNECTED));
    EXPECT_TRUE(conn.is_valid_state_transition(ConnectionState::SENDING, ConnectionState::DISCONNECTING));
    EXPECT_FALSE(conn.is_valid_state_transition(ConnectionState::SENDING, ConnectionState::PROCESSING));

    // DISCONNECTING 的合法转换
    EXPECT_TRUE(conn.is_valid_state_transition(ConnectionState::DISCONNECTING, ConnectionState::DISCONNECTING));
    EXPECT_TRUE(conn.is_valid_state_transition(ConnectionState::DISCONNECTING, ConnectionState::DISCONNECTED));
    EXPECT_FALSE(conn.is_valid_state_transition(ConnectionState::DISCONNECTING, ConnectionState::CONNECTED));

    // DISCONNECTED 的合法转换
    EXPECT_TRUE(conn.is_valid_state_transition(ConnectionState::DISCONNECTED, ConnectionState::DISCONNECTED));
    EXPECT_FALSE(conn.is_valid_state_transition(ConnectionState::DISCONNECTED, ConnectionState::CONNECTED));
}

// 补充测试：验证is_valid_state_transition返回值
// 此测试直接验证状态转换规则的正确性，不依赖实际transition_to行为
TEST(ConnectionTest, IsValidStateTransitionRules) {
    MockTimeSource timeSource;
    [[maybe_unused]] Connection conn(1, 5, 443, &timeSource);

    // 注意：is_valid_state_transition是private方法，这里通过实际场景验证
    // 合法转换路径
    Connection conn2(2, 6, 443, &timeSource);
    conn2.transition_to(ConnectionState::TLS_HANDSHAKING);
    EXPECT_EQ(conn2.get_state(), ConnectionState::TLS_HANDSHAKING);
    conn2.transition_to(ConnectionState::CONNECTED);
    EXPECT_EQ(conn2.get_state(), ConnectionState::CONNECTED);
    conn2.transition_to(ConnectionState::RECEIVING);
    EXPECT_EQ(conn2.get_state(), ConnectionState::RECEIVING);
    conn2.transition_to(ConnectionState::PROCESSING);
    EXPECT_EQ(conn2.get_state(), ConnectionState::PROCESSING);
    conn2.transition_to(ConnectionState::SENDING);
    EXPECT_EQ(conn2.get_state(), ConnectionState::SENDING);
    conn2.transition_to(ConnectionState::CONNECTED);
    EXPECT_EQ(conn2.get_state(), ConnectionState::CONNECTED);
}

// Conn_UT_006: 设置并获取Client信息
TEST(ConnectionTest, SetAndGetClientInfo) {
    MockTimeSource timeSource;
    Connection conn(1, 5, 443, &timeSource);

    conn.set_client_info("127.0.0.1", 54321);
    EXPECT_EQ(conn.get_client_ip(), "127.0.0.1");
    EXPECT_EQ(conn.get_client_port(), 54321);

    const ClientInfo& info = conn.get_client_info();
    EXPECT_EQ(info.client_ip, "127.0.0.1");
    EXPECT_EQ(info.client_port, 54321);
}

// 测试ClientInfo中connection_id和server_port初始化 (CONN-006)
TEST(ConnectionTest, ClientInfoInitialization) {
    MockTimeSource timeSource;
    const uint64_t expected_id = 12345;
    const uint16_t expected_port = 8443;
    Connection conn(expected_id, 5, expected_port, &timeSource);

    const ClientInfo& info = conn.get_client_info();
    EXPECT_EQ(info.connection_id, expected_id);
    EXPECT_EQ(info.server_port, expected_port);
    EXPECT_EQ(conn.get_server_port(), expected_port);
}

// Conn_UT_007: 获取基本属性
TEST(ConnectionTest, GetBasicProperties) {
    MockTimeSource timeSource;
    Connection conn(100, 5, 443, &timeSource);

    EXPECT_EQ(conn.get_id(), 100ULL);
    EXPECT_EQ(conn.get_fd(), 5);
    EXPECT_EQ(conn.get_server_port(), 443);
}

// Conn_UT_008: 未超时
TEST(ConnectionTest, NotTimeoutWithinThreshold) {
    auto mockTime = std::make_unique<MockTimeSource>();
    MockTimeSource* timePtr = mockTime.get();
    Connection conn(1, 5, 443, timePtr);

    conn.update_last_activity();
    timePtr->advance_time(500);
    EXPECT_FALSE(conn.is_timeout(1000));
}

// Conn_UT_009: 超时
TEST(ConnectionTest, TimeoutWhenExceedsThreshold) {
    auto mockTime = std::make_unique<MockTimeSource>();
    MockTimeSource* timePtr = mockTime.get();
    Connection conn(1, 5, 443, timePtr);

    conn.update_last_activity();
    timePtr->advance_time(150);
    EXPECT_TRUE(conn.is_timeout(100));
}

// 测试DISCONNECTED状态下is_timeout返回false (CONN-009)
TEST(ConnectionTest, IsTimeoutReturnsFalseWhenDisconnected) {
    auto mockTime = std::make_unique<MockTimeSource>();
    MockTimeSource* timePtr = mockTime.get();
    Connection conn(1, 5, 443, timePtr);

    conn.update_last_activity();
    timePtr->advance_time(500);

    // 在DISCONNECTED前应该可以超时
    EXPECT_TRUE(conn.is_timeout(100));

    // 转为DISCONNECTED
    conn.transition_to(ConnectionState::DISCONNECTING);
    conn.transition_to(ConnectionState::DISCONNECTED);

    // DISCONNECTED后即使时间再前进也不应超时
    timePtr->advance_time(10000);
    EXPECT_FALSE(conn.is_timeout(100));
    EXPECT_FALSE(conn.is_timeout(1));
}

// Conn_UT_010: 回调未超时
TEST(ConnectionTest, CallbackNotTimeoutWithinThreshold) {
    auto mockTime = std::make_unique<MockTimeSource>();
    MockTimeSource* timePtr = mockTime.get();
    Connection conn(1, 5, 443, timePtr);

    conn.set_callback_start_time();
    timePtr->advance_time(1000);
    EXPECT_FALSE(conn.is_callback_timeout(5000));
}

// Conn_UT_011: 回调完成后不再超时
TEST(ConnectionTest, CallbackCompleteDisablesTimeoutCheck) {
    MockTimeSource timeSource;
    Connection conn(1, 5, 443, &timeSource);

    conn.set_callback_start_time();
    conn.on_callback_complete();
    EXPECT_FALSE(conn.is_callback_timeout(100));
}

// Conn_UT_012: 获取缓冲区
TEST(ConnectionTest, GetBuffersReturnsValidReferences) {
    MockTimeSource timeSource;
    Connection conn(1, 5, 443, &timeSource);

    utils::Buffer& readBuf = conn.get_read_buffer();
    utils::Buffer& writeBuf = conn.get_write_buffer();

    // 验证可以写入数据
    const char* testData = "test";
    readBuf.write(reinterpret_cast<const uint8_t*>(testData), 4);
    writeBuf.write(reinterpret_cast<const uint8_t*>(testData), 4);

    EXPECT_EQ(readBuf.readable_bytes(), 4ULL);
    EXPECT_EQ(writeBuf.readable_bytes(), 4ULL);
}

// Conn_UT_013: 设置/获取ProtocolHandler
TEST(ConnectionTest, SetAndGetProtocolHandler) {
    MockTimeSource timeSource;
    Connection conn(1, 5, 443, &timeSource);

    auto handler = std::make_unique<MockProtocolHandler>();
    MockProtocolHandler* handlerPtr = handler.get();

    conn.set_protocol_handler(std::move(handler));
    EXPECT_EQ(conn.get_protocol_handler(), handlerPtr);
}

// 测试get_protocol_handler()的const重载 (CONN-003)
TEST(ConnectionTest, GetProtocolHandlerConstOverload) {
    MockTimeSource timeSource;
    Connection conn(1, 5, 443, &timeSource);

    auto handler = std::make_unique<MockProtocolHandler>();
    MockProtocolHandler* handlerPtr = handler.get();
    conn.set_protocol_handler(std::move(handler));

    // 测试非const版本
    ProtocolHandler* non_const_ptr = conn.get_protocol_handler();
    EXPECT_EQ(non_const_ptr, handlerPtr);

    // 测试const版本
    const Connection& const_conn = conn;
    const ProtocolHandler* const_ptr = const_conn.get_protocol_handler();
    EXPECT_EQ(const_ptr, handlerPtr);
}

// Conn_UT_014: 状态变化回调
TEST(ConnectionTest, StateChangeCallbackInvoked) {
    MockTimeSource timeSource;
    Connection conn(1, 5, 443, &timeSource);

    MockConnectionCallback callback;
    conn.set_state_callback(&callback);

    conn.transition_to(ConnectionState::TLS_HANDSHAKING);
    EXPECT_EQ(callback.call_count_, 1);
    EXPECT_EQ(callback.last_old_state_, ConnectionState::ACCEPTING);
    EXPECT_EQ(callback.last_new_state_, ConnectionState::TLS_HANDSHAKING);
}

// 测试状态回调为空指针时的边界情况 (CONN-008)
TEST(ConnectionTest, TransitionToWithNullCallbackDoesNotCrash) {
    MockTimeSource timeSource;
    Connection conn(1, 5, 443, &timeSource);

    // 明确不设置回调（保持nullptr）
    conn.set_state_callback(nullptr);

    // 执行多次状态转换，验证不会崩溃
    conn.transition_to(ConnectionState::TLS_HANDSHAKING);
    EXPECT_EQ(conn.get_state(), ConnectionState::TLS_HANDSHAKING);

    conn.transition_to(ConnectionState::CONNECTED);
    EXPECT_EQ(conn.get_state(), ConnectionState::CONNECTED);

    conn.transition_to(ConnectionState::DISCONNECTING);
    conn.transition_to(ConnectionState::DISCONNECTED);
    EXPECT_EQ(conn.get_state(), ConnectionState::DISCONNECTED);
}

// Conn_UT_015: 关闭连接
TEST(ConnectionTest, CloseConnectionSetsStateAndFd) {
    MockTimeSource timeSource;
    Connection conn(1, 5, 443, &timeSource);

    conn.transition_to(ConnectionState::TLS_HANDSHAKING);
    conn.transition_to(ConnectionState::CONNECTED);

    conn.close();
    EXPECT_EQ(conn.get_state(), ConnectionState::DISCONNECTED);
    EXPECT_EQ(conn.get_fd(), -1);
}

// 测试close()多次调用的幂等性 (CONN-004)
TEST(ConnectionTest, CloseMultipleTimesIsIdempotent) {
    MockTimeSource timeSource;
    Connection conn(1, 5, 443, &timeSource);

    conn.transition_to(ConnectionState::TLS_HANDSHAKING);
    conn.transition_to(ConnectionState::CONNECTED);

    // 第一次调用close
    conn.close();
    EXPECT_EQ(conn.get_state(), ConnectionState::DISCONNECTED);
    EXPECT_EQ(conn.get_fd(), -1);

    // 第二次调用close - 不应崩溃或产生问题
    conn.close();
    EXPECT_EQ(conn.get_state(), ConnectionState::DISCONNECTED);
    EXPECT_EQ(conn.get_fd(), -1);

    // 第三次调用close - 仍应安全
    conn.close();
    EXPECT_EQ(conn.get_state(), ConnectionState::DISCONNECTED);
}

// 补充测试：ProtocolHandler::close() 被调用
TEST(ConnectionTest, CloseInvokesProtocolHandlerClose) {
    MockTimeSource timeSource;
    Connection conn(1, 5, 443, &timeSource);

    auto handler = std::make_unique<MockProtocolHandler>();
    MockProtocolHandler* handlerPtr = handler.get();
    conn.set_protocol_handler(std::move(handler));

    conn.close();
    EXPECT_TRUE(handlerPtr->closed_);
}

// ==================== ConnectionManager 测试 ====================

// ConnMgr_UT_001: 创建连接
TEST(ConnectionManagerTest, CreateConnectionReturnsValidPointer) {
    ConnectionManager manager;
    Connection* conn = manager.create_connection(TEST_FD_1, TEST_PORT);
    EXPECT_NE(conn, nullptr);
    EXPECT_GE(conn->get_id(), 1ULL);
}

// ConnMgr_UT_002: 查找存在的连接
TEST(ConnectionManagerTest, GetExistingConnection) {
    ConnectionManager manager;
    Connection* conn1 = manager.create_connection(TEST_FD_1, TEST_PORT);
    uint64_t id = conn1->get_id();

    Connection* conn2 = manager.get_connection(id);
    EXPECT_EQ(conn1, conn2);
}

// ConnMgr_UT_003: 查找不存在的连接
TEST(ConnectionManagerTest, GetNonExistentConnectionReturnsNull) {
    ConnectionManager manager;
    Connection* conn = manager.get_connection(99999);
    EXPECT_EQ(conn, nullptr);
}

// ConnMgr_UT_004: 删除连接
TEST(ConnectionManagerTest, RemoveConnectionDeletesIt) {
    ConnectionManager manager;
    Connection* conn = manager.create_connection(TEST_FD_1, TEST_PORT);
    uint64_t id = conn->get_id();

    manager.remove_connection(id);
    Connection* found = manager.get_connection(id);
    EXPECT_EQ(found, nullptr);
}

// ConnMgr_UT_005: 获取连接数
TEST(ConnectionManagerTest, GetConnectionCount) {
    ConnectionManager manager;
    EXPECT_EQ(manager.get_connection_count(), 0U);

    manager.create_connection(TEST_FD_1, TEST_PORT);
    manager.create_connection(TEST_FD_2, TEST_PORT);
    manager.create_connection(TEST_FD_3, TEST_PORT);

    EXPECT_EQ(manager.get_connection_count(), 3U);
}

// ConnMgr_UT_006: 遍历连接
TEST(ConnectionManagerTest, ForEachConnectionIteratesAll) {
    ConnectionManager manager;
    manager.create_connection(TEST_FD_1, TEST_PORT);
    manager.create_connection(TEST_FD_2, TEST_PORT);

    int count = 0;
    manager.for_each_connection([&count](Connection&) {
        count++;
    });

    EXPECT_EQ(count, 2);
}

// 测试for_each_connection在回调中修改连接表 (CONN-010)
TEST(ConnectionManagerTest, ForEachConnectionSafeToModifyInCallback) {
    ConnectionManager manager;
    Connection* conn1 = manager.create_connection(TEST_FD_1, TEST_PORT);
    Connection* conn2 = manager.create_connection(TEST_FD_2, TEST_PORT);
    Connection* conn3 = manager.create_connection(TEST_FD_3, TEST_PORT);
    uint64_t id1 = conn1->get_id();
    uint64_t id2 = conn2->get_id();
    [[maybe_unused]] uint64_t id3 = conn3->get_id();

    int visit_count = 0;
    std::vector<uint64_t> visited_ids;

    manager.for_each_connection([&](Connection& conn) {
        visit_count++;
        visited_ids.push_back(conn.get_id());

        // 在回调中删除一个连接
        if (conn.get_id() == id1) {
            manager.remove_connection(id2);
        }

        // 在回调中添加一个新连接
        if (conn.get_id() == id1) {
            manager.create_connection(TEST_FD_4, TEST_PORT);
        }
    });

    // 应该遍历了初始的3个连接（先收集后调用）
    EXPECT_EQ(visit_count, 3);

    // 验证删除生效（id2应该不在了）
    EXPECT_EQ(manager.get_connection(id2), nullptr);

    // 验证新增连接存在（新连接在收集后添加，不会在本次遍历中访问）
    EXPECT_EQ(manager.get_connection_count(), 3U);  // 3初始 -1删除 +1新增 = 3
}

// ConnMgr_UT_007: 超时检测 + 锁外回调
TEST(ConnectionManagerTest, CheckTimeoutsDetectsAndCallsback) {
    auto mockTime = std::make_unique<MockTimeSource>();
    MockTimeSource* timePtr = mockTime.get();
    ConnectionManager manager(std::move(mockTime));

    // 创建两个连接
    Connection* conn1 = manager.create_connection(TEST_FD_1, TEST_PORT);
    Connection* conn2 = manager.create_connection(TEST_FD_2, TEST_PORT);

    // 让时间前进，使 conn1 超时（先更新 conn2 的活动时间）
    timePtr->advance_time(100);
    conn2->update_last_activity();
    timePtr->advance_time(250);

    int timeoutCount = 0;
    uint64_t timeoutId = 0;
    manager.check_timeouts(300, 30000, [&](Connection& conn) {
        timeoutCount++;
        timeoutId = conn.get_id();
    });

    EXPECT_EQ(timeoutCount, 1);
    EXPECT_EQ(timeoutId, conn1->get_id());
}

// 测试check_timeouts中回调期间调用remove_connection的安全性 (CONN-002)
TEST(ConnectionManagerTest, CheckTimeoutsSafeToRemoveConnectionInCallback) {
    auto mockTime = std::make_unique<MockTimeSource>();
    MockTimeSource* timePtr = mockTime.get();
    ConnectionManager manager(std::move(mockTime));

    // 创建多个连接
    Connection* conn1 = manager.create_connection(TEST_FD_1, TEST_PORT);
    Connection* conn2 = manager.create_connection(TEST_FD_2, TEST_PORT);
    Connection* conn3 = manager.create_connection(TEST_FD_3, TEST_PORT);
    uint64_t id1 = conn1->get_id();
    uint64_t id2 = conn2->get_id();
    uint64_t id3 = conn3->get_id();

    // 让所有连接都超时
    timePtr->advance_time(500);

    int callback_count = 0;
    std::vector<uint64_t> callback_ids;
    bool accessed_id2_after_remove = false;

    manager.check_timeouts(100, 30000, [&](Connection& conn) {
        callback_count++;
        callback_ids.push_back(conn.get_id());

        // 在回调中删除当前连接和另一个连接
        if (conn.get_id() == id1) {
            manager.remove_connection(id1);  // 删除自己
            manager.remove_connection(id2);  // 删除另一个
        }

        // 验证conn指针仍然有效（可以访问成员）
        // 即使从map中erase了，unique_ptr还在回调的栈帧中保持对象存活
        EXPECT_GE(conn.get_id(), 1ULL);
        EXPECT_NE(conn.get_fd(), -1);  // fd还没被关闭（除非调用close）

        // 尝试访问id2（已经被remove了，但如果在回调列表中还会被访问）
        if (conn.get_id() == id2) {
            accessed_id2_after_remove = true;
        }
    });

    // 应该有3个回调（先收集后调用）
    EXPECT_EQ(callback_count, 3);

    // 验证连接已被删除
    EXPECT_EQ(manager.get_connection(id1), nullptr);
    EXPECT_EQ(manager.get_connection(id2), nullptr);
    EXPECT_NE(manager.get_connection(id3), nullptr);
}

// ConnMgr_UT_008: 清空所有连接
TEST(ConnectionManagerTest, ClearAllRemovesAllConnections) {
    ConnectionManager manager;
    manager.create_connection(TEST_FD_1, TEST_PORT);
    manager.create_connection(TEST_FD_2, TEST_PORT);
    manager.create_connection(TEST_FD_3, TEST_PORT);

    EXPECT_EQ(manager.get_connection_count(), 3U);
    manager.clear_all();
    EXPECT_EQ(manager.get_connection_count(), 0U);
}

// ConnMgr_UT_009: 注入TimeSource
TEST(ConnectionManagerTest, UseInjectedTimeSource) {
    auto mockTime = std::make_unique<MockTimeSource>();
    MockTimeSource* timePtr = mockTime.get();
    ConnectionManager manager(std::move(mockTime));

    timePtr->set_time(12345);
    Connection* conn = manager.create_connection(TEST_FD_1, TEST_PORT);

    timePtr->set_time(12345 + 500);
    EXPECT_FALSE(conn->is_timeout(1000));

    timePtr->set_time(12345 + 1500);
    EXPECT_TRUE(conn->is_timeout(1000));
}

// 补充测试：回调超时检测
TEST(ConnectionManagerTest, CallbackTimeoutDetection) {
    auto mockTime = std::make_unique<MockTimeSource>();
    MockTimeSource* timePtr = mockTime.get();
    ConnectionManager manager(std::move(mockTime));

    Connection* conn = manager.create_connection(TEST_FD_1, TEST_PORT);
    conn->set_callback_start_time();

    timePtr->advance_time(500);
    int timeoutCount = 0;
    manager.check_timeouts(30000, 1000, [&](Connection&) {
        timeoutCount++;
    });
    EXPECT_EQ(timeoutCount, 0);

    timePtr->advance_time(600);
    manager.check_timeouts(30000, 1000, [&](Connection&) {
        timeoutCount++;
    });
    EXPECT_EQ(timeoutCount, 1);
}

// ==================== 新增功能测试 ====================

// 测试 connection_state_to_string 函数 (CONN-002)
TEST(ConnectionTest, ConnectionStateToStringWorks) {
    EXPECT_STREQ(connection_state_to_string(ConnectionState::ACCEPTING), "ACCEPTING");
    EXPECT_STREQ(connection_state_to_string(ConnectionState::TLS_HANDSHAKING), "TLS_HANDSHAKING");
    EXPECT_STREQ(connection_state_to_string(ConnectionState::CONNECTED), "CONNECTED");
    EXPECT_STREQ(connection_state_to_string(ConnectionState::RECEIVING), "RECEIVING");
    EXPECT_STREQ(connection_state_to_string(ConnectionState::PROCESSING), "PROCESSING");
    EXPECT_STREQ(connection_state_to_string(ConnectionState::SENDING), "SENDING");
    EXPECT_STREQ(connection_state_to_string(ConnectionState::DISCONNECTING), "DISCONNECTING");
    EXPECT_STREQ(connection_state_to_string(ConnectionState::DISCONNECTED), "DISCONNECTED");

    // 测试未知状态值
    auto unknown_state = static_cast<ConnectionState>(99);
    EXPECT_STREQ(connection_state_to_string(unknown_state), "UNKNOWN");
}

// 测试 is_fd_valid 方法 (CONN-005)
TEST(ConnectionTest, IsFdValidWorks) {
    MockTimeSource timeSource;
    Connection conn(1, 5, 443, &timeSource);

    // 初始状态 fd 有效
    EXPECT_TRUE(conn.is_fd_valid());
    EXPECT_EQ(conn.get_fd(), 5);

    // 关闭连接后 fd 无效
    conn.close();
    EXPECT_FALSE(conn.is_fd_valid());
    EXPECT_EQ(conn.get_fd(), -1);
}

// 测试 for_each_connection const 重载 (CONN-011)
TEST(ConnectionManagerTest, ForEachConnectionConstOverloadWorks) {
    ConnectionManager manager;
    manager.create_connection(TEST_FD_1, TEST_PORT);
    manager.create_connection(TEST_FD_2, TEST_PORT);

    int count = 0;
    const ConnectionManager& const_manager = manager;

    // 测试 const 版本
    const_manager.for_each_connection([&count](const Connection& conn) {
        count++;
        EXPECT_GE(conn.get_id(), 1ULL);
        // 验证可以调用 const 方法
        (void)conn.get_state();
        (void)conn.get_id();
    });

    EXPECT_EQ(count, 2);
}

// 测试 timeout_ms 为 0 的边界情况 (CONN-008)
TEST(ConnectionTest, TimeoutMsZeroBoundaryCase) {
    auto mockTime = std::make_unique<MockTimeSource>();
    MockTimeSource* timePtr = mockTime.get();
    Connection conn(1, 5, 443, timePtr);

    // 设置初始时间
    timePtr->set_time(1000);
    conn.update_last_activity();

    // 时间相同不超时 (timeout_ms=0)
    EXPECT_FALSE(conn.is_timeout(0));

    // 时间前进 1ms，超时 (timeout_ms=0)
    timePtr->set_time(1001);
    EXPECT_TRUE(conn.is_timeout(0));

    // 测试回调超时
    timePtr->set_time(2000);
    conn.set_callback_start_time();

    // 时间相同不超时 (timeout_ms=0)
    EXPECT_FALSE(conn.is_callback_timeout(0));

    // 时间前进 1ms，超时 (timeout_ms=0)
    timePtr->set_time(2001);
    EXPECT_TRUE(conn.is_callback_timeout(0));
}

} // namespace https_server_sim

// 文件结束
