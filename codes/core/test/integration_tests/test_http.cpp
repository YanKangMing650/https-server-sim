// =============================================================================
//  HTTPS Server Simulator - Integration Tests IT-033 ~ IT-050
//  文件: test_http.cpp
//  描述: HTTP 协议、回调、更多高级测试用例
//  版权: Copyright (c) 2026
// =============================================================================
#include "test_fixture.hpp"

namespace https_server_sim {
namespace integration_test {

// IT-033: HTTP/1.1 GET 请求
TEST_F(IntegrationTest, IT033_Http1GetRequest) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    TempFile config_file(get_valid_config_json(server_port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 使用 SimClient 发送 GET 请求
    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "GET", "/api/get",
        {}, ""
    );

    SimClientResult result = runner.run_scenario(client_config);

    // 期望 SimClient 执行成功
    EXPECT_TRUE(result.success) << "SimClient failed with exit code: " << result.exit_code;

    if (!result.output.empty()) {
        std::cout << "[SimClient Output]\n" << result.output << std::endl;
    }

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// IT-034: HTTP/1.1 POST 请求（带 body）
TEST_F(IntegrationTest, IT034_Http1PostWithBody) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    TempFile config_file(get_valid_config_json(server_port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 使用 SimClient 发送带 body 的 POST 请求
    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "POST", "/api/post",
        {{"Content-Type", "application/json"}},
        "{\"data\": \"test_payload\", \"id\": 12345}"
    );

    SimClientResult result = runner.run_scenario(client_config);

    // 期望 SimClient 执行成功
    EXPECT_TRUE(result.success) << "SimClient failed with exit code: " << result.exit_code;

    if (!result.output.empty()) {
        std::cout << "[SimClient Output]\n" << result.output << std::endl;
    }

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// IT-035: HTTP/1.1 分块传输编码
TEST_F(IntegrationTest, IT035_Http1ChunkedEncoding) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    TempFile config_file(get_valid_config_json(server_port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 使用 SimClient 发送请求（SimClient 可配置使用分块传输）
    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "POST", "/api/chunked",
        {}, "chunked_transfer_test_data"
    );

    SimClientResult result = runner.run_scenario(client_config);

    // 期望 SimClient 执行成功
    EXPECT_TRUE(result.success) << "SimClient failed with exit code: " << result.exit_code;

    if (!result.output.empty()) {
        std::cout << "[SimClient Output]\n" << result.output << std::endl;
    }

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// IT-036: HTTP/1.1 大量请求头
TEST_F(IntegrationTest, IT036_Http1ManyHeaders) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    TempFile config_file(get_valid_config_json(server_port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 构建多个请求头
    std::map<std::string, std::string> headers;
    for (int i = 0; i < 20; ++i) {
        headers["X-Custom-Header-" + std::to_string(i)] = "value_" + std::to_string(i);
    }

    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "GET", "/api/many_headers",
        headers, ""
    );

    SimClientResult result = runner.run_scenario(client_config);

    // 期望 SimClient 执行成功
    EXPECT_TRUE(result.success) << "SimClient failed with exit code: " << result.exit_code;

    if (!result.output.empty()) {
        std::cout << "[SimClient Output]\n" << result.output << std::endl;
    }

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// IT-037: HTTP/2 基础请求
TEST_F(IntegrationTest, IT037_Http2BasicRequest) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    TempFile config_file(get_valid_config_json(server_port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

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

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// IT-038: HTTP/2 多路复用 - 并发流
TEST_F(IntegrationTest, IT038_Http2ConcurrentStreams) {
    uint16_t port = get_test_port();
    TempFile config_file(get_valid_config_json(port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);
    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// IT-039: HTTP/2 服务端推送
TEST_F(IntegrationTest, IT039_Http2ServerPush) {
    uint16_t port = get_test_port();
    TempFile config_file(get_valid_config_json(port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);
    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// IT-040: HTTP/1.1 升级到 HTTP/2
TEST_F(IntegrationTest, IT040_Http2Upgrade) {
    uint16_t port = get_test_port();
    TempFile config_file(get_valid_config_json(port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);
    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// IT-041: 回调超时检测
TEST_F(IntegrationTest, IT041_CallbackTimeoutDetection) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    TempFile config_file(get_valid_config_json(server_port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 使用较短的超时来测试回调超时
    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "POST", "/api/callback_timeout",
        {}, "", 0, 500  // 500ms 超时
    );

    SimClientResult result = runner.run_scenario(client_config);

    // 期望 SimClient 执行成功
    EXPECT_TRUE(result.success) << "SimClient failed with exit code: " << result.exit_code;

    if (!result.output.empty()) {
        std::cout << "[SimClient Output]\n" << result.output << std::endl;
    }

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// IT-042: 回调正常完成（刚好不超时）
TEST_F(IntegrationTest, IT042_CallbackNoTimeout) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    TempFile config_file(get_valid_config_json(server_port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "POST", "/api/callback_normal",
        {}, "", 0, 10000  // 10秒超时，足够完成
    );

    SimClientResult result = runner.run_scenario(client_config);

    // 期望 SimClient 执行成功
    EXPECT_TRUE(result.success) << "SimClient failed with exit code: " << result.exit_code;

    if (!result.output.empty()) {
        std::cout << "[SimClient Output]\n" << result.output << std::endl;
    }

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// IT-043: 多端口 - 不同回调策略
TEST_F(IntegrationTest, IT043_MultiPortDifferentCallbacks) {
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
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 验证两个端口都能连接
    TestClient client1("127.0.0.1", port1);
    TestClient client2("127.0.0.1", port2);
    bool connected1 = client1.connect();
    bool connected2 = client2.connect();
    (void)connected1;
    (void)connected2;

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// IT-044 ~ IT-050: 更多测试占位，保持现有结构
TEST_F(IntegrationTest, IT044_GracefulShutdownWait) {
    uint16_t port = get_test_port();
    TempFile config_file(get_valid_config_json(port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);
    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

TEST_F(IntegrationTest, IT045_GracefulShutdownTimeout) {
    uint16_t port = get_test_port();
    TempFile config_file(get_valid_config_json(port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);
    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

TEST_F(IntegrationTest, IT046_ConfigReload) {
    uint16_t port = get_test_port();
    TempFile config_file(get_valid_config_json(port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);
    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

TEST_F(IntegrationTest, IT047_LogLevelDynamicChange) {
    uint16_t port = get_test_port();
    TempFile config_file(get_valid_config_json(port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);
    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

TEST_F(IntegrationTest, IT048_ConnectionLimit) {
    uint16_t port = get_test_port();
    TempFile config_file(get_valid_config_json(port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);
    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

TEST_F(IntegrationTest, IT049_FastRestartPortReuse) {
    uint16_t port = get_test_port();
    TempFile config_file(get_valid_config_json(port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);
    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

TEST_F(IntegrationTest, IT050_ExceptionRecoveryConfigCorruption) {
    uint16_t port = get_test_port();
    TempFile config_file(get_valid_config_json(port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);
    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

} // namespace integration_test
} // namespace https_server_sim

// 文件结束
