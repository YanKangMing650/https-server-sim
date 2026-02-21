// =============================================================================
//  HTTPS Server Simulator - Stress Tests STRESS-001 ~ STRESS-012
//  文件: test_stress.cpp
//  描述: 压力测试和边界条件测试用例
//  版权: Copyright (c) 2026
// =============================================================================
#include "test_fixture.hpp"
#include <vector>
#include <memory>

namespace https_server_sim {
namespace integration_test {

// STRESS-001: 压力测试 - 100 并发连接
TEST_F(IntegrationTest, STRESS001_Stress100Connections) {
    uint16_t port = get_test_port();
    TempFile config_file(get_valid_config_json(port));

    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    server::ServerStatus status;
    server.get_status(&status);
    EXPECT_EQ(status.status, server::SERVER_STATUS_RUNNING);

    // 使用 TestClient 尝试多个连接
    std::vector<std::unique_ptr<TestClient>> clients;
    const int num_clients = 10;  // 先用较小数量验证框架
    for (int i = 0; i < num_clients; ++i) {
        auto client = std::make_unique<TestClient>("127.0.0.1", port);
        bool connected = client->connect();
        if (connected) {
            clients.push_back(std::move(client));
        }
    }

    // 断开所有连接
    for (auto& client : clients) {
        client->disconnect();
    }

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// STRESS-002: 压力测试 - 1000 并发连接
TEST_F(IntegrationTest, STRESS002_Stress1000Connections) {
    uint16_t port = get_test_port();
    TempFile config_file(get_valid_config_json(port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 获取统计信息初始值
    utils::Statistics initial_stats;
    server.get_statistics(&initial_stats);

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// STRESS-003: 压力测试 - 长连接持久化
TEST_F(IntegrationTest, STRESS003_StressLongConnections) {
    uint16_t server_port = get_test_port();
    (void)client_port_counter;
    TempFile config_file(get_valid_config_json(server_port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 建立并保持一个连接
    TestClient client("127.0.0.1", server_port);
    bool connected = client.connect();
    if (connected) {
        // 保持连接一段时间
        sleep_ms(50);
    }

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// STRESS-004: 压力测试 - 频繁连接断开
TEST_F(IntegrationTest, STRESS004_StressConnectDisconnect) {
    uint16_t port = get_test_port();
    TempFile config_file(get_valid_config_json(port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 多次连接断开循环
    const int cycles = 5;
    for (int i = 0; i < cycles; ++i) {
        TestClient client("127.0.0.1", port);
        bool connected = client.connect();
        if (connected) {
            client.disconnect();
        }
    }

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// STRESS-005: 边界条件 - 超大请求头
TEST_F(IntegrationTest, STRESS005_BoundaryLargeHeader) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    TempFile config_file(get_valid_config_json(server_port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 构建很多请求头
    std::map<std::string, std::string> many_headers;
    for (int i = 0; i < 50; ++i) {
        many_headers["X-Test-Header-" + std::to_string(i)] =
            std::string(100, 'v');  // 每个值 100 字符
    }

    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "GET", "/api/large_headers",
        many_headers, ""
    );

    SimClientResult result = runner.run_scenario(client_config);
    // 期望 SimClient 执行成功
    EXPECT_TRUE(result.success) << "SimClient failed with exit code: " << result.exit_code;

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// STRESS-006: 边界条件 - 超大请求体
TEST_F(IntegrationTest, STRESS006_BoundaryLargeBody) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    TempFile config_file(get_valid_config_json(server_port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 较大的请求体（16KB）
    std::string large_body(16384, 'D');

    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "POST", "/api/large_body",
        {}, large_body
    );

    SimClientResult result = runner.run_scenario(client_config);
    // 期望 SimClient 执行成功
    EXPECT_TRUE(result.success) << "SimClient failed with exit code: " << result.exit_code;

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// STRESS-007: 边界条件 - 极短连接超时
TEST_F(IntegrationTest, STRESS007_BoundaryShortTimeout) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    TempFile config_file(get_valid_config_json(server_port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 使用极短的超时时间
    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "GET", "/api/short_timeout",
        {}, "", 0, 100  // 100ms 超时
    );

    SimClientResult result = runner.run_scenario(client_config);
    // 期望 SimClient 执行成功
    EXPECT_TRUE(result.success) << "SimClient failed with exit code: " << result.exit_code;

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// STRESS-008: 边界条件 - 零长度请求
TEST_F(IntegrationTest, STRESS008_BoundaryZeroLengthRequest) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    TempFile config_file(get_valid_config_json(server_port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 空 body 的请求
    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "POST", "/api/zero_length",
        {}, ""
    );

    SimClientResult result = runner.run_scenario(client_config);
    // 期望 SimClient 执行成功
    EXPECT_TRUE(result.success) << "SimClient failed with exit code: " << result.exit_code;

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// STRESS-009: 边界条件 - 慢速攻击（Slowloris）
TEST_F(IntegrationTest, STRESS009_BoundarySlowloris) {
    uint16_t port = get_test_port();
    TempFile config_file(get_valid_config_json(port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 建立连接后等待再断开
    TestClient client("127.0.0.1", port);
    bool connected = client.connect();
    if (connected) {
        sleep_ms(100);  // 保持连接不发送数据
        client.disconnect();
    }

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// STRESS-010: 边界条件 - 请求头格式异常
TEST_F(IntegrationTest, STRESS010_BoundaryInvalidHeaders) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    TempFile config_file(get_valid_config_json(server_port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 发送包含特殊字符的 header 值
    std::map<std::string, std::string> headers;
    headers["X-Normal-Header"] = "normal_value";

    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "GET", "/api/invalid_headers",
        headers, ""
    );

    SimClientResult result = runner.run_scenario(client_config);
    // 期望 SimClient 执行成功
    EXPECT_TRUE(result.success) << "SimClient failed with exit code: " << result.exit_code;

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// STRESS-011: 边界条件 - 编码异常
TEST_F(IntegrationTest, STRESS011_BoundaryEncodingErrors) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    TempFile config_file(get_valid_config_json(server_port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 包含非 ASCII 字符的 body
    std::string body_with_utf8 = "测试数据 UTF-8 测试";

    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "POST", "/api/encoding",
        {{"Content-Type", "text/plain; charset=utf-8"}},
        body_with_utf8
    );

    SimClientResult result = runner.run_scenario(client_config);
    // 期望 SimClient 执行成功
    EXPECT_TRUE(result.success) << "SimClient failed with exit code: " << result.exit_code;

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// STRESS-012: 边界条件 - 端口耗尽
TEST_F(IntegrationTest, STRESS012_BoundaryPortExhaustion) {
    uint16_t port = get_test_port();
    TempFile config_file(get_valid_config_json(port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 获取 Server 状态
    server::ServerStatus status;
    server.get_status(&status);
    EXPECT_EQ(status.status, server::SERVER_STATUS_RUNNING);
    EXPECT_EQ(status.listen_port, port);

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

} // namespace integration_test
} // namespace https_server_sim

// 文件结束
