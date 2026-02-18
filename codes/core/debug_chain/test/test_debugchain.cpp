// HTTPS Server 模拟器 - DebugChain单元测试
// 模块：DebugChain - 职责链管理器
// 用途：验证DebugChain模块的功能正确性

#include <gtest/gtest.h>
#include <cstring>
#include <vector>
#include "debug_chain/debug_config.hpp"
#include "debug_chain/debug_context.hpp"
#include "debug_chain/debug_handler.hpp"
#include "debug_chain/debug_chain.hpp"

// 使用命名空间
using namespace https_server_sim::debug_chain;

// ========== DebugConfig测试 ==========

TEST(DebugConfigTest, DefaultValues) {
    DebugConfig config;

    EXPECT_FALSE(config.enabled);
    EXPECT_EQ(config.delay_ms, 0u);
    EXPECT_FALSE(config.force_disconnect);
    EXPECT_FALSE(config.log_packet);
    EXPECT_EQ(config.http_status, 200);
}

TEST(DebugConfigTest, InitDebugConfigFunction) {
    DebugConfig config;
    config.enabled = true;
    config.delay_ms = 1000;
    config.force_disconnect = true;
    config.log_packet = true;
    config.http_status = 404;

    // 使用InitDebugConfig重置
    InitDebugConfig(&config);

    EXPECT_FALSE(config.enabled);
    EXPECT_EQ(config.delay_ms, 0u);
    EXPECT_FALSE(config.force_disconnect);
    EXPECT_FALSE(config.log_packet);
    EXPECT_EQ(config.http_status, 200);
}

// ========== DebugContext测试 ==========

TEST(DebugContextTest, DefaultValues) {
    DebugContext ctx;

    EXPECT_TRUE(ctx.request_data.empty());
    EXPECT_TRUE(ctx.response_data.empty());
    EXPECT_EQ(ctx.override_http_status, 0);
    EXPECT_FALSE(ctx.skip_callback);
    EXPECT_FALSE(ctx.disconnect_after);
    // 验证config也有正确的默认值
    EXPECT_FALSE(ctx.config.enabled);
    EXPECT_EQ(ctx.config.http_status, 200);
}

// ========== 测试夹具(Fixture) ==========

class DebugChainTest : public ::testing::Test {
public:
    // 测试辅助用自定义Handler的状态（public以便辅助函数访问）
    static int customHandlerCallCount;
    static int customHandlerReturnValue;
    static std::vector<int> executionOrder;
    static bool handler1Called;
    static bool handler2Called;

protected:
    void SetUp() override {
        customHandlerCallCount = 0;
        customHandlerReturnValue = 0;
        executionOrder.clear();
        handler1Called = false;
        handler2Called = false;
    }

    void TearDown() override {
    }
};

// 静态成员初始化
int DebugChainTest::customHandlerCallCount = 0;
int DebugChainTest::customHandlerReturnValue = 0;
std::vector<int> DebugChainTest::executionOrder;
bool DebugChainTest::handler1Called = false;
bool DebugChainTest::handler2Called = false;

// ========== 测试辅助用自定义Handler ==========

static int custom_handle_request(DebugHandler* self,
                                 const ClientContext* ctx,
                                 const DebugConfig* config,
                                 DebugContext* debug_ctx) {
    (void)self;
    (void)ctx;
    (void)config;
    (void)debug_ctx;
    DebugChainTest::customHandlerCallCount++;
    return DebugChainTest::customHandlerReturnValue;
}

static int custom_handle_response(DebugHandler* self,
                                  const ClientContext* ctx,
                                  const DebugConfig* config,
                                  DebugContext* debug_ctx) {
    (void)self;
    (void)ctx;
    (void)config;
    (void)debug_ctx;
    DebugChainTest::customHandlerCallCount++;
    return DebugChainTest::customHandlerReturnValue;
}

static void custom_destroy(DebugHandler* self) {
    if (self != nullptr) {
        delete self;
    }
}

static DebugHandler* create_custom_handler(const char* name, int priority) {
    DebugHandler* handler = new DebugHandler();
    if (handler == nullptr) {
        return nullptr;
    }
    handler->name = name;
    handler->priority = priority;
    handler->user_data = nullptr;
    handler->handle_request = custom_handle_request;
    handler->handle_response = custom_handle_response;
    handler->destroy = custom_destroy;
    return handler;
}

// ========== DebugChain测试 ==========

TEST_F(DebugChainTest, RegisterHandler) {
    DebugChain chain;
    DebugHandler* handler = create_custom_handler("TestHandler", 100);
    ASSERT_NE(handler, nullptr);

    int ret = chain.register_handler(handler);
    EXPECT_EQ(ret, DebugChain::kRetSuccess);
}

TEST_F(DebugChainTest, RegisterDuplicateHandlerReturnsAlreadyExists) {
    DebugChain chain;
    DebugHandler* handler1 = create_custom_handler("TestHandler", 100);
    DebugHandler* handler2 = create_custom_handler("TestHandler", 200);
    ASSERT_NE(handler1, nullptr);
    ASSERT_NE(handler2, nullptr);

    int ret1 = chain.register_handler(handler1);
    EXPECT_EQ(ret1, DebugChain::kRetSuccess);

    // 重复注册应该返回kRetAlreadyExists(-3)
    int ret2 = chain.register_handler(handler2);
    EXPECT_EQ(ret2, DebugChain::kRetAlreadyExists);

    // 清理handler2（因为没有被注册，不会被chain析构）
    handler2->destroy(handler2);
}

TEST_F(DebugChainTest, UnregisterHandler) {
    DebugChain chain;
    DebugHandler* handler = create_custom_handler("TestHandler", 100);
    ASSERT_NE(handler, nullptr);

    chain.register_handler(handler);

    int ret = chain.unregister_handler("TestHandler");
    EXPECT_EQ(ret, DebugChain::kRetSuccess);
}

TEST_F(DebugChainTest, UnregisterNonExistentHandlerReturnsNotFound) {
    DebugChain chain;
    // 未找到应该返回kRetNotFound(-2)
    int ret = chain.unregister_handler("NonExistent");
    EXPECT_EQ(ret, DebugChain::kRetNotFound);
}

TEST_F(DebugChainTest, HasHandler) {
    DebugChain chain;
    DebugHandler* handler = create_custom_handler("TestHandler", 100);
    ASSERT_NE(handler, nullptr);

    EXPECT_FALSE(chain.has_handler("TestHandler"));
    chain.register_handler(handler);
    EXPECT_TRUE(chain.has_handler("TestHandler"));
}

TEST_F(DebugChainTest, ConfigEnabledFalse) {
    DebugChain chain;
    ClientContext client_ctx = {};
    DebugConfig config;  // 使用默认构造，enabled=false
    DebugContext debug_ctx;

    int ret = chain.process_request(&client_ctx, &config, &debug_ctx);
    EXPECT_EQ(ret, DebugChain::kRetContinueChain);
}

TEST_F(DebugChainTest, RegisterNullptrReturnsInvalidParam) {
    DebugChain chain;
    int ret = chain.register_handler(nullptr);
    EXPECT_EQ(ret, DebugChain::kRetInvalidParam);
}

TEST_F(DebugChainTest, HandlerWithNullptrFuncPointersSkipped) {
    DebugChain chain;

    // 创建一个函数指针为nullptr的handler
    DebugHandler* handler = new DebugHandler();
    handler->name = "NullFuncHandler";
    handler->priority = 100;
    handler->user_data = nullptr;
    handler->handle_request = nullptr;
    handler->handle_response = nullptr;
    handler->destroy = custom_destroy;

    chain.register_handler(handler);

    ClientContext client_ctx = {};
    DebugConfig config;
    config.enabled = true;
    DebugContext debug_ctx;

    // 应该正常执行，不崩溃
    int ret = chain.process_request(&client_ctx, &config, &debug_ctx);
    EXPECT_EQ(ret, DebugChain::kRetContinueChain);
}

// ========== 测试执行顺序 ==========

static int order_handler_100_request(DebugHandler* self,
                                      const ClientContext* ctx,
                                      const DebugConfig* config,
                                      DebugContext* debug_ctx) {
    (void)self;
    (void)ctx;
    (void)config;
    (void)debug_ctx;
    DebugChainTest::executionOrder.push_back(100);
    return DebugChain::kRetContinueChain;
}

static int order_handler_200_request(DebugHandler* self,
                                      const ClientContext* ctx,
                                      const DebugConfig* config,
                                      DebugContext* debug_ctx) {
    (void)self;
    (void)ctx;
    (void)config;
    (void)debug_ctx;
    DebugChainTest::executionOrder.push_back(200);
    return DebugChain::kRetContinueChain;
}

static int order_handler_50_request(DebugHandler* self,
                                     const ClientContext* ctx,
                                     const DebugConfig* config,
                                     DebugContext* debug_ctx) {
    (void)self;
    (void)ctx;
    (void)config;
    (void)debug_ctx;
    DebugChainTest::executionOrder.push_back(50);
    return DebugChain::kRetContinueChain;
}

static DebugHandler* create_order_handler(const char* name, int priority,
                                           int (*handler_func)(DebugHandler*, const ClientContext*, const DebugConfig*, DebugContext*)) {
    DebugHandler* handler = new DebugHandler();
    if (handler == nullptr) {
        return nullptr;
    }
    handler->name = name;
    handler->priority = priority;
    handler->user_data = nullptr;
    handler->handle_request = handler_func;
    handler->handle_response = nullptr;
    handler->destroy = custom_destroy;
    return handler;
}

TEST_F(DebugChainTest, ExecutionOrderByPriority) {
    executionOrder.clear();

    DebugChain chain;
    DebugHandler* h100 = create_order_handler("H100", 100, order_handler_100_request);
    DebugHandler* h200 = create_order_handler("H200", 200, order_handler_200_request);
    DebugHandler* h50 = create_order_handler("H50", 50, order_handler_50_request);

    chain.register_handler(h100);
    chain.register_handler(h200);
    chain.register_handler(h50);

    ClientContext client_ctx = {};
    DebugConfig config;
    config.enabled = true;
    DebugContext debug_ctx;

    chain.process_request(&client_ctx, &config, &debug_ctx);

    ASSERT_EQ(executionOrder.size(), 3u);
    EXPECT_EQ(executionOrder[0], 50);
    EXPECT_EQ(executionOrder[1], 100);
    EXPECT_EQ(executionOrder[2], 200);
}

// ========== 测试Handler返回非0终止链 ==========

static int stopping_handler_request(DebugHandler* self,
                                     const ClientContext* ctx,
                                     const DebugConfig* config,
                                     DebugContext* debug_ctx) {
    (void)self;
    (void)ctx;
    (void)config;
    (void)debug_ctx;
    DebugChainTest::handler1Called = true;
    return DebugChain::kRetStopChain;  // 返回非0
}

static int next_handler_request(DebugHandler* self,
                                 const ClientContext* ctx,
                                 const DebugConfig* config,
                                 DebugContext* debug_ctx) {
    (void)self;
    (void)ctx;
    (void)config;
    (void)debug_ctx;
    DebugChainTest::handler2Called = true;
    return DebugChain::kRetContinueChain;
}

TEST_F(DebugChainTest, HandlerReturnValueStopsChain) {
    handler1Called = false;
    handler2Called = false;

    DebugChain chain;

    DebugHandler* handler1 = new DebugHandler();
    handler1->name = "StoppingHandler";
    handler1->priority = 100;
    handler1->user_data = nullptr;
    handler1->handle_request = stopping_handler_request;
    handler1->handle_response = nullptr;
    handler1->destroy = custom_destroy;

    DebugHandler* handler2 = new DebugHandler();
    handler2->name = "NextHandler";
    handler2->priority = 200;
    handler2->user_data = nullptr;
    handler2->handle_request = next_handler_request;
    handler2->handle_response = nullptr;
    handler2->destroy = custom_destroy;

    chain.register_handler(handler1);
    chain.register_handler(handler2);

    ClientContext client_ctx = {};
    DebugConfig config;
    config.enabled = true;
    DebugContext debug_ctx;

    int ret = chain.process_request(&client_ctx, &config, &debug_ctx);
    EXPECT_EQ(ret, DebugChain::kRetStopChain);
    EXPECT_TRUE(handler1Called);
    EXPECT_FALSE(handler2Called);
}

// ========== 默认Handler测试 ==========

TEST(DefaultHandlerTest, CreateDelayHandler) {
    DebugHandler* handler = create_delay_handler();
    ASSERT_NE(handler, nullptr);
    EXPECT_STREQ(handler->name, "DelayHandler");
    EXPECT_EQ(handler->priority, 100);
    EXPECT_NE(handler->handle_request, nullptr);
    EXPECT_NE(handler->handle_response, nullptr);
    EXPECT_NE(handler->destroy, nullptr);
    handler->destroy(handler);
}

TEST(DefaultHandlerTest, CreateDisconnectHandler) {
    DebugHandler* handler = create_disconnect_handler();
    ASSERT_NE(handler, nullptr);
    EXPECT_STREQ(handler->name, "DisconnectHandler");
    EXPECT_EQ(handler->priority, 200);
    EXPECT_NE(handler->handle_request, nullptr);
    EXPECT_NE(handler->handle_response, nullptr);
    EXPECT_NE(handler->destroy, nullptr);
    handler->destroy(handler);
}

TEST(DefaultHandlerTest, CreateLogHandler) {
    DebugHandler* handler = create_log_handler();
    ASSERT_NE(handler, nullptr);
    EXPECT_STREQ(handler->name, "LogHandler");
    EXPECT_EQ(handler->priority, 300);
    EXPECT_NE(handler->handle_request, nullptr);
    EXPECT_NE(handler->handle_response, nullptr);
    EXPECT_NE(handler->destroy, nullptr);
    handler->destroy(handler);
}

TEST(DefaultHandlerTest, CreateErrorCodeHandler) {
    DebugHandler* handler = create_errorcode_handler();
    ASSERT_NE(handler, nullptr);
    EXPECT_STREQ(handler->name, "ErrorCodeHandler");
    EXPECT_EQ(handler->priority, 400);
    EXPECT_NE(handler->handle_request, nullptr);
    EXPECT_NE(handler->handle_response, nullptr);
    EXPECT_NE(handler->destroy, nullptr);
    handler->destroy(handler);
}

// ========== DisconnectHandler功能测试 ==========

TEST(DisconnectHandlerTest, HandleRequestSetsDisconnectAfter) {
    DebugHandler* handler = create_disconnect_handler();
    ASSERT_NE(handler, nullptr);

    ClientContext client_ctx = {};
    DebugConfig config;
    config.enabled = true;
    config.force_disconnect = true;
    DebugContext debug_ctx;

    int ret = handler->handle_request(handler, &client_ctx, &config, &debug_ctx);
    EXPECT_EQ(ret, DebugChain::kRetStopChain);
    EXPECT_TRUE(debug_ctx.disconnect_after);

    handler->destroy(handler);
}

TEST(DisconnectHandlerTest, HandleResponseSetsDisconnectAfter) {
    DebugHandler* handler = create_disconnect_handler();
    ASSERT_NE(handler, nullptr);

    ClientContext client_ctx = {};
    DebugConfig config;
    config.enabled = true;
    config.force_disconnect = true;
    DebugContext debug_ctx;

    int ret = handler->handle_response(handler, &client_ctx, &config, &debug_ctx);
    EXPECT_EQ(ret, DebugChain::kRetStopChain);
    EXPECT_TRUE(debug_ctx.disconnect_after);

    handler->destroy(handler);
}

// ========== ErrorCodeHandler功能测试 ==========

TEST(ErrorCodeHandlerTest, HandleRequestSetsHttpStatus) {
    DebugHandler* handler = create_errorcode_handler();
    ASSERT_NE(handler, nullptr);

    ClientContext client_ctx = {};
    DebugConfig config;
    config.enabled = true;
    config.http_status = 404;
    DebugContext debug_ctx;

    int ret = handler->handle_request(handler, &client_ctx, &config, &debug_ctx);
    EXPECT_EQ(ret, DebugChain::kRetContinueChain);
    EXPECT_EQ(debug_ctx.override_http_status, 404);

    handler->destroy(handler);
}

TEST(ErrorCodeHandlerTest, HandleResponseOnlySetsIfZero) {
    DebugHandler* handler = create_errorcode_handler();
    ASSERT_NE(handler, nullptr);

    ClientContext client_ctx = {};
    DebugConfig config;
    config.enabled = true;
    config.http_status = 404;
    DebugContext debug_ctx;

    debug_ctx.override_http_status = 500;
    int ret = handler->handle_response(handler, &client_ctx, &config, &debug_ctx);
    EXPECT_EQ(ret, DebugChain::kRetContinueChain);
    EXPECT_EQ(debug_ctx.override_http_status, 500);  // 保持不变

    debug_ctx.override_http_status = 0;
    ret = handler->handle_response(handler, &client_ctx, &config, &debug_ctx);
    EXPECT_EQ(ret, DebugChain::kRetContinueChain);
    EXPECT_EQ(debug_ctx.override_http_status, 404);  // 设置为404

    handler->destroy(handler);
}

// ========== DelayHandler边界测试 ==========

TEST(DelayHandlerTest, DelayMsZero) {
    DebugHandler* handler = create_delay_handler();
    ASSERT_NE(handler, nullptr);

    ClientContext client_ctx = {};
    DebugConfig config;
    config.enabled = true;
    config.delay_ms = 0;
    DebugContext debug_ctx;

    // 应该立即返回，无延迟
    int ret = handler->handle_request(handler, &client_ctx, &config, &debug_ctx);
    EXPECT_EQ(ret, DebugChain::kRetContinueChain);

    handler->destroy(handler);
}

// ========== DebugHandlerRegistry C接口测试 ==========

TEST(DebugHandlerRegistryTest, CreateDestroy) {
    DebugHandlerRegistry* registry = debug_handler_registry_create();
    ASSERT_NE(registry, nullptr);

    DebugChain* chain = debug_handler_registry_get_chain(registry);
    EXPECT_NE(chain, nullptr);

    debug_handler_registry_destroy(registry);
}

TEST(DebugHandlerRegistryTest, DefaultHandlersRegistered) {
    DebugHandlerRegistry* registry = debug_handler_registry_create();
    ASSERT_NE(registry, nullptr);

    DebugChain* chain = debug_handler_registry_get_chain(registry);
    ASSERT_NE(chain, nullptr);

    EXPECT_TRUE(chain->has_handler("DelayHandler"));
    EXPECT_TRUE(chain->has_handler("DisconnectHandler"));
    EXPECT_TRUE(chain->has_handler("LogHandler"));
    EXPECT_TRUE(chain->has_handler("ErrorCodeHandler"));

    debug_handler_registry_destroy(registry);
}

TEST(DebugHandlerRegistryTest, RegisterDuplicateHandlerReturnsMinus3) {
    DebugHandlerRegistry* registry = debug_handler_registry_create();
    ASSERT_NE(registry, nullptr);

    DebugHandler* handler = create_delay_handler();
    ASSERT_NE(handler, nullptr);

    // 已存在应该返回-3
    int ret = debug_handler_registry_register(registry, handler);
    EXPECT_EQ(ret, DEBUG_HANDLER_REGISTRY_ALREADY_EXISTS);

    handler->destroy(handler);
    debug_handler_registry_destroy(registry);
}

TEST(DebugHandlerRegistryTest, UnregisterHandler) {
    DebugHandlerRegistry* registry = debug_handler_registry_create();
    ASSERT_NE(registry, nullptr);

    int ret = debug_handler_registry_unregister(registry, "LogHandler");
    EXPECT_EQ(ret, DEBUG_HANDLER_REGISTRY_SUCCESS);

    DebugChain* chain = debug_handler_registry_get_chain(registry);
    EXPECT_FALSE(chain->has_handler("LogHandler"));

    debug_handler_registry_destroy(registry);
}

TEST(DebugHandlerRegistryTest, UnregisterNonExistentHandlerReturnsMinus2) {
    DebugHandlerRegistry* registry = debug_handler_registry_create();
    ASSERT_NE(registry, nullptr);

    // 未找到应该返回-2
    int ret = debug_handler_registry_unregister(registry, "NonExistentHandler");
    EXPECT_EQ(ret, DEBUG_HANDLER_REGISTRY_NOT_FOUND);

    debug_handler_registry_destroy(registry);
}
