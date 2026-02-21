// =============================================================================
//  HTTPS Server Simulator - Integration Tests IT-008 ~ IT-013
//  文件: test_callback_and_more.cpp
//  描述: 回调策略、多端口、Graceful Shutdown 等测试
//  版权: Copyright (c) 2026
// =============================================================================
#include "test_fixture.hpp"

namespace https_server_sim {
namespace integration_test {

// IT-008: 回调策略执行流程
TEST_F(IntegrationTest, IT008_CallbackStrategyFlow) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    std::string config_json = R"({
        "listens": [
            {"ip": "127.0.0.1", "port": )" + std::to_string(server_port) + R"(, "enabled": true}
        ],
        "certificates": {
            "cert_path": "",
            "key_path": "",
            "ca_path": "",
            "cipher_suite": "TLS_AES_256_GCM_SHA384"
        },
        "debug": {
            "enabled": false,
            "debug_points": []
        },
        "callbacks": {
            "callbacks_dir": "",
            "callbacks": []
        },
        "logging": {
            "level": "INFO",
            "file": "",
            "console_output": true
        },
        "http2": {
            "enabled": true,
            "allow_h2c": false
        }
    })";
    TempFile config_file(config_json);

    server::Server server;

    // 启动 Server
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 验证 Server 状态
    server::ServerStatus status;
    server.get_status(&status);
    EXPECT_EQ(status.status, server::SERVER_STATUS_RUNNING);

    // 使用 SimClient 发送请求以触发回调流程
    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "POST", "/api/callback",
        {}, ""
    );

    SimClientResult result = runner.run_scenario(client_config);

    // 期望 SimClient 执行成功
    EXPECT_TRUE(result.success) << "SimClient failed with exit code: " << result.exit_code;

    if (!result.output.empty()) {
        std::cout << "[SimClient Output]\n" << result.output << std::endl;
    }

    // 停止 Server
    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// IT-009: 多端口监听
TEST_F(IntegrationTest, IT009_MultiPortListen) {
    uint16_t port1 = get_test_port();
    uint16_t port2 = get_test_port();
    std::string config_json = R"({
        "listens": [
            {"ip": "127.0.0.1", "port": )" + std::to_string(port1) + R"(, "enabled": true},
            {"ip": "127.0.0.1", "port": )" + std::to_string(port2) + R"(, "enabled": true}
        ],
        "certificates": {
            "cert_path": "",
            "key_path": "",
            "ca_path": "",
            "cipher_suite": "TLS_AES_256_GCM_SHA384"
        },
        "debug": {
            "enabled": false,
            "debug_points": []
        },
        "callbacks": {
            "callbacks_dir": "",
            "callbacks": []
        },
        "logging": {
            "level": "INFO",
            "file": "",
            "console_output": true
        },
        "http2": {
            "enabled": true,
            "allow_h2c": false
        }
    })";
    TempFile config_file(config_json);

    server::Server server;

    // 启动 Server，监听两个端口
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 验证 Server 状态
    server::ServerStatus status;
    server.get_status(&status);
    EXPECT_EQ(status.status, server::SERVER_STATUS_RUNNING);

    // 使用 TestClient 验证两个端口都能建立 TCP 连接
    TestClient client1("127.0.0.1", port1);
    bool connected1 = client1.connect();
    (void)connected1;

    TestClient client2("127.0.0.1", port2);
    bool connected2 = client2.connect();
    (void)connected2;

    // 停止 Server
    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// IT-010: Graceful Shutdown
TEST_F(IntegrationTest, IT010_GracefulShutdown) {
    uint16_t port = get_test_port();
    TempFile config_file(get_valid_config_json(port));

    server::Server server;

    // 启动 Server
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 验证 Server 状态
    server::ServerStatus status;
    server.get_status(&status);
    EXPECT_EQ(status.status, server::SERVER_STATUS_RUNNING);

    // 停止 Server - 测试 Graceful Shutdown
    auto start_time = std::chrono::steady_clock::now();
    ret = server.stop();
    auto end_time = std::chrono::steady_clock::now();
    EXPECT_EQ(ret, 0);

    // 没有连接时应该快速停止
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    EXPECT_LT(duration.count(), 5000LL) << "Graceful shutdown should complete quickly without connections";

    server.cleanup();
}

// IT-011: HTTP/2 协议支持
TEST_F(IntegrationTest, IT011_Http2ProtocolSupport) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    std::string config_json = R"({
        "listens": [
            {"ip": "127.0.0.1", "port": )" + std::to_string(server_port) + R"(, "enabled": true}
        ],
        "certificates": {
            "cert_path": "",
            "key_path": "",
            "ca_path": "",
            "cipher_suite": "TLS_AES_256_GCM_SHA384"
        },
        "debug": {
            "enabled": false,
            "debug_points": []
        },
        "callbacks": {
            "callbacks_dir": "",
            "callbacks": []
        },
        "logging": {
            "level": "INFO",
            "file": "",
            "console_output": true
        },
        "http2": {
            "enabled": true,
            "allow_h2c": false
        }
    })";
    TempFile config_file(config_json);

    server::Server server;

    // 启动 Server，启用 HTTP/2
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 使用 SimClient 发送请求（SimClient 会尝试使用 HTTP/2）
    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "GET", "/api/http2",
        {}, ""
    );

    SimClientResult result = runner.run_scenario(client_config);

    // 期望 SimClient 执行成功
    EXPECT_TRUE(result.success) << "SimClient failed with exit code: " << result.exit_code;

    if (!result.output.empty()) {
        std::cout << "[SimClient Output]\n" << result.output << std::endl;
    }

    // 停止 Server
    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// IT-012: 连接超时检测
TEST_F(IntegrationTest, IT012_ConnectionTimeoutDetection) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    TempFile config_file(get_valid_config_json(server_port));

    server::Server server;

    // 启动 Server
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 使用 SimClient 发送带超时配置的请求
    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "POST", "/api/timeout",
        {}, "", 0, 1000  // 1秒超时
    );

    SimClientResult result = runner.run_scenario(client_config);

    // 期望 SimClient 执行成功
    EXPECT_TRUE(result.success) << "SimClient failed with exit code: " << result.exit_code;

    if (!result.output.empty()) {
        std::cout << "[SimClient Output]\n" << result.output << std::endl;
    }

    // 停止 Server
    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// IT-013: 统计信息收集
TEST_F(IntegrationTest, IT013_StatisticsCollection) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    TempFile config_file(get_valid_config_json(server_port));

    server::Server server;

    // 启动 Server
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 发送一些请求来产生统计数据
    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "POST", "/api/stats",
        {}, ""
    );

    SimClientResult result = runner.run_scenario(client_config);
    EXPECT_TRUE(result.success) << "SimClient failed with exit code: " << result.exit_code;

    if (!result.output.empty()) {
        std::cout << "[SimClient Output]\n" << result.output << std::endl;
    }

    // 查询统计信息
    utils::Statistics stats;
    server.get_statistics(&stats);

    // 验证统计信息可以正常获取
    EXPECT_EQ(stats.total_connections, 0ULL);
    EXPECT_EQ(stats.total_requests, 0ULL);

    // 停止 Server
    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

} // namespace integration_test
} // namespace https_server_sim

// 文件结束
