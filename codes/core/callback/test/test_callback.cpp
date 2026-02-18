// =============================================================================
//  HTTPS Server Simulator - Callback Module
//  文件: test_callback.cpp
//  描述: Callback模块单元测试
//  版权: Copyright (c) 2026
// =============================================================================
#include <gtest/gtest.h>
#include "callback/callback.hpp"
#include "callback/callback.h"
#include <thread>
#include <vector>
#include <atomic>

namespace https_server_sim {
namespace test {

// ==================== 测试辅助变量（用于避免滥用token字段） ====================
// 全局测试指针，用于死锁测试中传递manager指针
static CallbackStrategyManager* g_test_manager = nullptr;

// ==================== 测试辅助函数 ====================

// 测试用的解析回调函数
static uint32_t test_parse_func(const ClientContext* ctx,
                                 const uint8_t* in,
                                 uint32_t inLen) {
    (void)ctx;
    (void)in;
    (void)inLen;
    return 12345;
}

// 测试用的响应回调函数
static uint32_t test_reply_func(const ClientContext* ctx,
                                 uint8_t* out,
                                 uint32_t* outLen) {
    (void)ctx;
    if (out != nullptr && outLen != nullptr && *outLen >= 5) {
        out[0] = 'H';
        out[1] = 'e';
        out[2] = 'l';
        out[3] = 'l';
        out[4] = 'o';
        *outLen = 5;
    }
    return 54321;
}

// 用于死锁预防测试的回调（回调内再次调用manager操作）
// 使用全局测试变量传递manager指针，避免滥用token字段
static uint32_t deadlock_test_parse_func(const ClientContext* ctx,
                                           const uint8_t* in,
                                           uint32_t inLen) {
    (void)ctx;
    (void)in;
    (void)inLen;
    // 从全局测试变量获取manager指针并调用get_callback，验证不会死锁
    if (g_test_manager != nullptr) {
        g_test_manager->get_callback(8443);
    }
    return 99999;
}

// ==================== C++内部方法测试 ====================

class CallbackStrategyManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager_ = std::make_unique<CallbackStrategyManager>();
        g_test_manager = manager_.get();
    }

    void TearDown() override {
        g_test_manager = nullptr;
        manager_.reset();
    }

    std::unique_ptr<CallbackStrategyManager> manager_;
};

// Callback_UC001: 正常注册单个策略
// 用例编号: Callback_UC001
TEST_F(CallbackStrategyManagerTest, RegisterSingleStrategy) {
    CallbackStrategy strategy;
    strategy.name = "TestStrategy";
    strategy.port = 8443;
    strategy.parse = test_parse_func;
    strategy.reply = test_reply_func;

    int ret = manager_->register_callback(&strategy);
    EXPECT_EQ(ret, CALLBACK_SUCCESS);
    EXPECT_EQ(manager_->get_callback_count(), 1u);
}

// Callback_UC002: 重复注册同一端口
// 用例编号: Callback_UC002
TEST_F(CallbackStrategyManagerTest, RegisterDuplicatePort) {
    CallbackStrategy strategy1;
    strategy1.name = "TestStrategy1";
    strategy1.port = 8443;
    strategy1.parse = test_parse_func;
    strategy1.reply = test_reply_func;

    CallbackStrategy strategy2;
    strategy2.name = "TestStrategy2";
    strategy2.port = 8443;
    strategy2.parse = test_parse_func;
    strategy2.reply = test_reply_func;

    int ret1 = manager_->register_callback(&strategy1);
    EXPECT_EQ(ret1, CALLBACK_SUCCESS);

    int ret2 = manager_->register_callback(&strategy2);
    EXPECT_EQ(ret2, CALLBACK_ERR_PORT_EXISTS);
}

// Callback_UC003: NULL strategy指针
// 用例编号: Callback_UC003
TEST_F(CallbackStrategyManagerTest, RegisterNullStrategy) {
    int ret = manager_->register_callback(nullptr);
    EXPECT_EQ(ret, CALLBACK_ERR_INVALID_PARAM);
}

// Callback_UC004: NULL name
// 用例编号: Callback_UC004
TEST_F(CallbackStrategyManagerTest, RegisterNullName) {
    CallbackStrategy strategy;
    strategy.name = nullptr;
    strategy.port = 8443;
    strategy.parse = test_parse_func;
    strategy.reply = test_reply_func;

    int ret = manager_->register_callback(&strategy);
    EXPECT_EQ(ret, CALLBACK_ERR_INVALID_PARAM);
}

// 补充测试: 空字符串name
// 用例编号: Callback_UC004a
TEST_F(CallbackStrategyManagerTest, RegisterEmptyName) {
    CallbackStrategy strategy;
    strategy.name = "";
    strategy.port = 8443;
    strategy.parse = test_parse_func;
    strategy.reply = test_reply_func;

    int ret = manager_->register_callback(&strategy);
    EXPECT_EQ(ret, CALLBACK_ERR_INVALID_PARAM);
}

// Callback_UC005: port=0
// 用例编号: Callback_UC005
TEST_F(CallbackStrategyManagerTest, RegisterPortZero) {
    CallbackStrategy strategy;
    strategy.name = "TestStrategy";
    strategy.port = 0;
    strategy.parse = test_parse_func;
    strategy.reply = test_reply_func;

    int ret = manager_->register_callback(&strategy);
    EXPECT_EQ(ret, CALLBACK_ERR_INVALID_PARAM);
}

// Callback_UC006: port=65535
// 用例编号: Callback_UC006
TEST_F(CallbackStrategyManagerTest, RegisterPortMax) {
    CallbackStrategy strategy;
    strategy.name = "TestStrategy";
    strategy.port = 65535;
    strategy.parse = test_parse_func;
    strategy.reply = test_reply_func;

    int ret = manager_->register_callback(&strategy);
    EXPECT_EQ(ret, CALLBACK_SUCCESS);
}

// Callback_UC007: NULL parse函数
// 用例编号: Callback_UC007
TEST_F(CallbackStrategyManagerTest, RegisterNullParse) {
    CallbackStrategy strategy;
    strategy.name = "TestStrategy";
    strategy.port = 8443;
    strategy.parse = nullptr;
    strategy.reply = test_reply_func;

    int ret = manager_->register_callback(&strategy);
    EXPECT_EQ(ret, CALLBACK_ERR_INVALID_PARAM);
}

// Callback_UC008: NULL reply函数
// 用例编号: Callback_UC008
TEST_F(CallbackStrategyManagerTest, RegisterNullReply) {
    CallbackStrategy strategy;
    strategy.name = "TestStrategy";
    strategy.port = 8443;
    strategy.parse = test_parse_func;
    strategy.reply = nullptr;

    int ret = manager_->register_callback(&strategy);
    EXPECT_EQ(ret, CALLBACK_ERR_INVALID_PARAM);
}

// Callback_UC009: 查询已注册端口
// 用例编号: Callback_UC009
TEST_F(CallbackStrategyManagerTest, GetRegisteredPort) {
    CallbackStrategy strategy;
    strategy.name = "TestStrategy";
    strategy.port = 8443;
    strategy.parse = test_parse_func;
    strategy.reply = test_reply_func;

    manager_->register_callback(&strategy);

    const CallbackStrategy* retrieved = manager_->get_callback(8443);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->port, 8443);
    EXPECT_STREQ(retrieved->name, "TestStrategy");
}

// Callback_UC010: 查询未注册端口
// 用例编号: Callback_UC010
TEST_F(CallbackStrategyManagerTest, GetUnregisteredPort) {
    const CallbackStrategy* retrieved = manager_->get_callback(9999);
    EXPECT_EQ(retrieved, nullptr);
}

// Callback_UC011: 注销已注册端口
// 用例编号: Callback_UC011
TEST_F(CallbackStrategyManagerTest, DeregisterRegisteredPort) {
    CallbackStrategy strategy;
    strategy.name = "TestStrategy";
    strategy.port = 8443;
    strategy.parse = test_parse_func;
    strategy.reply = test_reply_func;

    manager_->register_callback(&strategy);
    EXPECT_EQ(manager_->get_callback_count(), 1u);

    int ret = manager_->deregister_callback(8443);
    EXPECT_EQ(ret, CALLBACK_SUCCESS);
    EXPECT_EQ(manager_->get_callback_count(), 0u);
}

// Callback_UC012: 注销未注册端口
// 用例编号: Callback_UC012
TEST_F(CallbackStrategyManagerTest, DeregisterUnregisteredPort) {
    int ret = manager_->deregister_callback(9999);
    EXPECT_EQ(ret, CALLBACK_ERR_PORT_NOT_FOUND);
}

// Callback_UC013: 清空多个策略
// 用例编号: Callback_UC013
TEST_F(CallbackStrategyManagerTest, ClearMultipleStrategies) {
    for (uint16_t port = 8001; port <= 8003; ++port) {
        CallbackStrategy strategy;
        strategy.name = "TestStrategy";
        strategy.port = port;
        strategy.parse = test_parse_func;
        strategy.reply = test_reply_func;
        manager_->register_callback(&strategy);
    }
    EXPECT_EQ(manager_->get_callback_count(), 3u);

    manager_->clear();
    EXPECT_EQ(manager_->get_callback_count(), 0u);

    for (uint16_t port = 8001; port <= 8003; ++port) {
        EXPECT_EQ(manager_->get_callback(port), nullptr);
    }
}

// Callback_UC014: 统计策略数量
// 用例编号: Callback_UC014
TEST_F(CallbackStrategyManagerTest, GetStrategyCount) {
    EXPECT_EQ(manager_->get_callback_count(), 0u);

    for (uint16_t port = 8001; port <= 8010; ++port) {
        CallbackStrategy strategy;
        strategy.name = "TestStrategy";
        strategy.port = port;
        strategy.parse = test_parse_func;
        strategy.reply = test_reply_func;
        manager_->register_callback(&strategy);
    }
    EXPECT_EQ(manager_->get_callback_count(), 10u);
}

// Callback_UC016: 注册多个不同端口策略
// 用例编号: Callback_UC016
TEST_F(CallbackStrategyManagerTest, RegisterMultiplePorts) {
    for (uint16_t port = 8001; port <= 8010; ++port) {
        CallbackStrategy strategy;
        strategy.name = "TestStrategy";
        strategy.port = port;
        strategy.parse = test_parse_func;
        strategy.reply = test_reply_func;
        int ret = manager_->register_callback(&strategy);
        EXPECT_EQ(ret, CALLBACK_SUCCESS);
    }
    EXPECT_EQ(manager_->get_callback_count(), 10u);

    for (uint16_t port = 8001; port <= 8010; ++port) {
        const CallbackStrategy* s = manager_->get_callback(port);
        EXPECT_NE(s, nullptr);
        EXPECT_EQ(s->port, port);
    }
}

// Callback_UC018: 调用已注册的解析回调
// 用例编号: Callback_UC018
TEST_F(CallbackStrategyManagerTest, InvokeParseCallback) {
    CallbackStrategy strategy;
    strategy.name = "TestStrategy";
    strategy.port = 8443;
    strategy.parse = test_parse_func;
    strategy.reply = test_reply_func;
    manager_->register_callback(&strategy);

    ClientContext ctx;
    ctx.connection_id = 123;
    ctx.client_ip = "127.0.0.1";
    ctx.client_port = 12345;
    ctx.server_port = 8443;
    ctx.token = nullptr;

    uint8_t in_data[] = "TestData";
    uint32_t out_result = 0;

    int ret = manager_->invoke_parse_callback(8443, &ctx, in_data, sizeof(in_data), &out_result);
    EXPECT_EQ(ret, CALLBACK_SUCCESS);
    EXPECT_EQ(out_result, 12345u);
}

// Callback_UC019: 调用已注册的响应回调
// 用例编号: Callback_UC019
TEST_F(CallbackStrategyManagerTest, InvokeReplyCallback) {
    CallbackStrategy strategy;
    strategy.name = "TestStrategy";
    strategy.port = 8443;
    strategy.parse = test_parse_func;
    strategy.reply = test_reply_func;
    manager_->register_callback(&strategy);

    ClientContext ctx;
    ctx.connection_id = 123;
    ctx.client_ip = "127.0.0.1";
    ctx.client_port = 12345;
    ctx.server_port = 8443;
    ctx.token = nullptr;

    uint8_t out_buffer[100] = {0};
    uint32_t out_len = sizeof(out_buffer);
    uint32_t out_result = 0;

    int ret = manager_->invoke_reply_callback(8443, &ctx, out_buffer, &out_len, &out_result);
    EXPECT_EQ(ret, CALLBACK_SUCCESS);
    EXPECT_EQ(out_result, 54321u);
    EXPECT_EQ(out_len, 5u);
    EXPECT_EQ(out_buffer[0], 'H');
    EXPECT_EQ(out_buffer[1], 'e');
    EXPECT_EQ(out_buffer[2], 'l');
    EXPECT_EQ(out_buffer[3], 'l');
    EXPECT_EQ(out_buffer[4], 'o');
}

// Callback_UC020: 回调内重入注册/查询（死锁预防）
// 用例编号: Callback_UC020
TEST_F(CallbackStrategyManagerTest, DeadlockPrevention) {
    CallbackStrategy strategy;
    strategy.name = "DeadlockTest";
    strategy.port = 8443;
    strategy.parse = deadlock_test_parse_func;
    strategy.reply = test_reply_func;
    manager_->register_callback(&strategy);

    ClientContext ctx;
    ctx.connection_id = 123;
    ctx.client_ip = "127.0.0.1";
    ctx.client_port = 12345;
    ctx.server_port = 8443;
    ctx.token = nullptr;  // 不再使用token字段传递指针

    uint32_t out_result = 0;
    int ret = manager_->invoke_parse_callback(8443, &ctx, nullptr, 0, &out_result);

    EXPECT_EQ(ret, CALLBACK_SUCCESS);
    EXPECT_EQ(out_result, 99999u);
}

// ==================== invoke_parse_callback错误路径测试 ====================

// 用例编号: Callback_ErrPath_001
TEST_F(CallbackStrategyManagerTest, InvokeParseCallbackNullCtx) {
    CallbackStrategy strategy;
    strategy.name = "TestStrategy";
    strategy.port = 8443;
    strategy.parse = test_parse_func;
    strategy.reply = test_reply_func;
    manager_->register_callback(&strategy);

    uint8_t in_data[] = "TestData";
    uint32_t out_result = 0;

    int ret = manager_->invoke_parse_callback(8443, nullptr, in_data, sizeof(in_data), &out_result);
    EXPECT_EQ(ret, CALLBACK_ERR_INVALID_PARAM);
}

// 用例编号: Callback_ErrPath_002
TEST_F(CallbackStrategyManagerTest, InvokeParseCallbackNullOutResult) {
    CallbackStrategy strategy;
    strategy.name = "TestStrategy";
    strategy.port = 8443;
    strategy.parse = test_parse_func;
    strategy.reply = test_reply_func;
    manager_->register_callback(&strategy);

    ClientContext ctx;
    ctx.connection_id = 123;
    ctx.client_ip = "127.0.0.1";
    ctx.client_port = 12345;
    ctx.server_port = 8443;
    ctx.token = nullptr;

    uint8_t in_data[] = "TestData";

    int ret = manager_->invoke_parse_callback(8443, &ctx, in_data, sizeof(in_data), nullptr);
    EXPECT_EQ(ret, CALLBACK_ERR_INVALID_PARAM);
}

// 用例编号: Callback_ErrPath_003
TEST_F(CallbackStrategyManagerTest, InvokeParseCallbackNullInWithNonZeroLen) {
    CallbackStrategy strategy;
    strategy.name = "TestStrategy";
    strategy.port = 8443;
    strategy.parse = test_parse_func;
    strategy.reply = test_reply_func;
    manager_->register_callback(&strategy);

    ClientContext ctx;
    ctx.connection_id = 123;
    ctx.client_ip = "127.0.0.1";
    ctx.client_port = 12345;
    ctx.server_port = 8443;
    ctx.token = nullptr;

    uint32_t out_result = 0;

    int ret = manager_->invoke_parse_callback(8443, &ctx, nullptr, 100, &out_result);
    EXPECT_EQ(ret, CALLBACK_ERR_INVALID_PARAM);
}

// 用例编号: Callback_ErrPath_004
TEST_F(CallbackStrategyManagerTest, InvokeParseCallbackUnregisteredPort) {
    ClientContext ctx;
    ctx.connection_id = 123;
    ctx.client_ip = "127.0.0.1";
    ctx.client_port = 12345;
    ctx.server_port = 9999;
    ctx.token = nullptr;

    uint8_t in_data[] = "TestData";
    uint32_t out_result = 0;

    int ret = manager_->invoke_parse_callback(9999, &ctx, in_data, sizeof(in_data), &out_result);
    EXPECT_EQ(ret, CALLBACK_ERR_STRATEGY_NOT_FOUND);
}

// ==================== invoke_reply_callback错误路径测试 ====================

// 用例编号: Callback_ErrPath_005
TEST_F(CallbackStrategyManagerTest, InvokeReplyCallbackNullCtx) {
    CallbackStrategy strategy;
    strategy.name = "TestStrategy";
    strategy.port = 8443;
    strategy.parse = test_parse_func;
    strategy.reply = test_reply_func;
    manager_->register_callback(&strategy);

    uint8_t out_buffer[100] = {0};
    uint32_t out_len = sizeof(out_buffer);
    uint32_t out_result = 0;

    int ret = manager_->invoke_reply_callback(8443, nullptr, out_buffer, &out_len, &out_result);
    EXPECT_EQ(ret, CALLBACK_ERR_INVALID_PARAM);
}

// 用例编号: Callback_ErrPath_006
TEST_F(CallbackStrategyManagerTest, InvokeReplyCallbackNullOut) {
    CallbackStrategy strategy;
    strategy.name = "TestStrategy";
    strategy.port = 8443;
    strategy.parse = test_parse_func;
    strategy.reply = test_reply_func;
    manager_->register_callback(&strategy);

    ClientContext ctx;
    ctx.connection_id = 123;
    ctx.client_ip = "127.0.0.1";
    ctx.client_port = 12345;
    ctx.server_port = 8443;
    ctx.token = nullptr;

    uint32_t out_len = 100;
    uint32_t out_result = 0;

    int ret = manager_->invoke_reply_callback(8443, &ctx, nullptr, &out_len, &out_result);
    EXPECT_EQ(ret, CALLBACK_ERR_INVALID_PARAM);
}

// 用例编号: Callback_ErrPath_007
TEST_F(CallbackStrategyManagerTest, InvokeReplyCallbackNullOutLen) {
    CallbackStrategy strategy;
    strategy.name = "TestStrategy";
    strategy.port = 8443;
    strategy.parse = test_parse_func;
    strategy.reply = test_reply_func;
    manager_->register_callback(&strategy);

    ClientContext ctx;
    ctx.connection_id = 123;
    ctx.client_ip = "127.0.0.1";
    ctx.client_port = 12345;
    ctx.server_port = 8443;
    ctx.token = nullptr;

    uint8_t out_buffer[100] = {0};
    uint32_t out_result = 0;

    int ret = manager_->invoke_reply_callback(8443, &ctx, out_buffer, nullptr, &out_result);
    EXPECT_EQ(ret, CALLBACK_ERR_INVALID_PARAM);
}

// 用例编号: Callback_ErrPath_008
TEST_F(CallbackStrategyManagerTest, InvokeReplyCallbackNullOutResult) {
    CallbackStrategy strategy;
    strategy.name = "TestStrategy";
    strategy.port = 8443;
    strategy.parse = test_parse_func;
    strategy.reply = test_reply_func;
    manager_->register_callback(&strategy);

    ClientContext ctx;
    ctx.connection_id = 123;
    ctx.client_ip = "127.0.0.1";
    ctx.client_port = 12345;
    ctx.server_port = 8443;
    ctx.token = nullptr;

    uint8_t out_buffer[100] = {0};
    uint32_t out_len = sizeof(out_buffer);

    int ret = manager_->invoke_reply_callback(8443, &ctx, out_buffer, &out_len, nullptr);
    EXPECT_EQ(ret, CALLBACK_ERR_INVALID_PARAM);
}

// 用例编号: Callback_ErrPath_009
TEST_F(CallbackStrategyManagerTest, InvokeReplyCallbackUnregisteredPort) {
    ClientContext ctx;
    ctx.connection_id = 123;
    ctx.client_ip = "127.0.0.1";
    ctx.client_port = 12345;
    ctx.server_port = 9999;
    ctx.token = nullptr;

    uint8_t out_buffer[100] = {0};
    uint32_t out_len = sizeof(out_buffer);
    uint32_t out_result = 0;

    int ret = manager_->invoke_reply_callback(9999, &ctx, out_buffer, &out_len, &out_result);
    EXPECT_EQ(ret, CALLBACK_ERR_STRATEGY_NOT_FOUND);
}

// Callback_UC015: 多线程并发注册查询
// 用例编号: Callback_UC015
TEST_F(CallbackStrategyManagerTest, MultithreadedAccess) {
    std::atomic<int> success_count{0};
    std::vector<std::thread> threads;

    // 5个线程注册不同端口
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([this, i, &success_count]() {
            uint16_t port = 9000 + static_cast<uint16_t>(i);
            CallbackStrategy strategy;
            strategy.name = "MultithreadTest";
            strategy.port = port;
            strategy.parse = test_parse_func;
            strategy.reply = test_reply_func;
            if (manager_->register_callback(&strategy) == CALLBACK_SUCCESS) {
                success_count++;
            }
        });
    }

    // 5个线程查询这些端口
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([this, i]() {
            uint16_t port = 9000 + static_cast<uint16_t>(i);
            // 循环查询多次
            for (int j = 0; j < 100; ++j) {
                manager_->get_callback(port);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count.load(), 5);
    EXPECT_EQ(manager_->get_callback_count(), 5u);
}

// 新增测试用例：多线程并发调用回调
// 用例编号: Callback_UC021（补充建议问题7）
TEST_F(CallbackStrategyManagerTest, MultithreadedCallbackInvoke) {
    CallbackStrategy strategy;
    strategy.name = "ConcurrentTest";
    strategy.port = 8443;
    strategy.parse = test_parse_func;
    strategy.reply = test_reply_func;
    manager_->register_callback(&strategy);

    std::atomic<int> invoke_success_count{0};
    std::vector<std::thread> threads;
    const int thread_count = 10;
    const int invokes_per_thread = 100;

    // 多个线程同时调用parse回调
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([this, i, &invoke_success_count]() {
            ClientContext ctx;
            ctx.connection_id = i;
            ctx.client_ip = "127.0.0.1";
            ctx.client_port = 10000 + i;
            ctx.server_port = 8443;
            ctx.token = nullptr;

            uint32_t out_result = 0;
            for (int j = 0; j < invokes_per_thread; ++j) {
                int ret = manager_->invoke_parse_callback(8443, &ctx, nullptr, 0, &out_result);
                if (ret == CALLBACK_SUCCESS && out_result == 12345u) {
                    invoke_success_count++;
                }
            }
        });
    }

    // 多个线程同时调用reply回调
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([this, i]() {
            ClientContext ctx;
            ctx.connection_id = thread_count + i;
            ctx.client_ip = "127.0.0.1";
            ctx.client_port = 20000 + i;
            ctx.server_port = 8443;
            ctx.token = nullptr;

            uint8_t out_buffer[100] = {0};
            uint32_t out_len = sizeof(out_buffer);
            uint32_t out_result = 0;
            for (int j = 0; j < invokes_per_thread; ++j) {
                manager_->invoke_reply_callback(8443, &ctx, out_buffer, &out_len, &out_result);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(invoke_success_count.load(), thread_count * invokes_per_thread);
}

// ==================== C接口测试 ====================

class CInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        registry_ = callback_registry_create();
        ASSERT_NE(registry_, nullptr);
    }

    void TearDown() override {
        if (registry_ != nullptr) {
            callback_registry_destroy(registry_);
            registry_ = nullptr;
        }
    }

    CallbackRegistry* registry_ = nullptr;
};

// Callback_UC017: C接口完整流程
// 用例编号: Callback_UC017
TEST_F(CInterfaceTest, CInterfaceCompleteFlow) {
    // 1. 构造有效的CallbackStrategy
    CallbackStrategy strategy;
    strategy.name = "TestCInterface";
    strategy.port = 8443;
    strategy.parse = test_parse_func;
    strategy.reply = test_reply_func;

    // 2. 注册策略
    int ret = callback_registry_register_strategy(registry_, &strategy);
    EXPECT_EQ(ret, CALLBACK_SUCCESS);

    // 3. 查询已注册端口
    const CallbackStrategy* retrieved = callback_registry_get_strategy(registry_, 8443);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->port, 8443);

    // 4. 查询未注册端口
    const CallbackStrategy* not_found = callback_registry_get_strategy(registry_, 9999);
    EXPECT_EQ(not_found, nullptr);
}

} // namespace test
} // namespace https_server_sim

// 文件结束
