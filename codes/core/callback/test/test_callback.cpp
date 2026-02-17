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
// 使用ClientContext::token字段传递manager指针，避免全局变量
static uint32_t deadlock_test_parse_func(const ClientContext* ctx,
                                           const uint8_t* in,
                                           uint32_t inLen) {
    (void)in;
    (void)inLen;
    // 从token字段获取manager指针并调用get_callback，验证不会死锁
    if (ctx != nullptr && ctx->token != nullptr) {
        auto* manager = reinterpret_cast<CallbackStrategyManager*>(
            const_cast<char*>(ctx->token));
        manager->get_callback(8443);
    }
    return 99999;
}

// ==================== C++内部方法测试 ====================

class CallbackStrategyManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager_ = std::make_unique<CallbackStrategyManager>();
    }

    void TearDown() override {
        manager_.reset();
    }

    std::unique_ptr<CallbackStrategyManager> manager_;
};

// Callback_UC001: 正常注册单个策略
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
TEST_F(CallbackStrategyManagerTest, RegisterNullStrategy) {
    int ret = manager_->register_callback(nullptr);
    EXPECT_EQ(ret, CALLBACK_ERR_INVALID_PARAM);
}

// Callback_UC004: NULL name
TEST_F(CallbackStrategyManagerTest, RegisterNullName) {
    CallbackStrategy strategy;
    strategy.name = nullptr;
    strategy.port = 8443;
    strategy.parse = test_parse_func;
    strategy.reply = test_reply_func;

    int ret = manager_->register_callback(&strategy);
    EXPECT_EQ(ret, CALLBACK_ERR_INVALID_PARAM);
}

// Callback_UC005: port=0
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
TEST_F(CallbackStrategyManagerTest, GetUnregisteredPort) {
    const CallbackStrategy* retrieved = manager_->get_callback(9999);
    EXPECT_EQ(retrieved, nullptr);
}

// Callback_UC011: 注销已注册端口
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
TEST_F(CallbackStrategyManagerTest, DeregisterUnregisteredPort) {
    int ret = manager_->deregister_callback(9999);
    EXPECT_EQ(ret, CALLBACK_ERR_PORT_NOT_FOUND);
}

// Callback_UC013: 清空多个策略
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
    // 使用token字段传递manager指针，避免使用全局变量
    ctx.token = reinterpret_cast<const char*>(manager_.get());

    uint32_t out_result = 0;
    int ret = manager_->invoke_parse_callback(8443, &ctx, nullptr, 0, &out_result);

    EXPECT_EQ(ret, CALLBACK_SUCCESS);
    EXPECT_EQ(out_result, 99999u);
}

// Callback_UC015: 多线程并发注册查询
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

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// 文件结束
