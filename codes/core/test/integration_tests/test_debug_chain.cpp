// =============================================================================
//  HTTPS Server Simulator - Integration Tests IT-005 ~ IT-007, IT-023 ~ IT-027
//  文件: test_debug_chain.cpp
//  描述: DebugChain 调测功能测试
//  版权: Copyright (c) 2026
// =============================================================================
#include "test_fixture.hpp"

namespace https_server_sim {
namespace integration_test {

// IT-005: DebugChain 延迟响应调测点
TEST_F(IntegrationTest, IT005_DebugChainDelayResponse) {
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
            "enabled": true,
            "debug_points": []
        },
        "callbacks": {
            "callbacks_dir": "",
            "callbacks": []
        },
        "logging": {
            "level": "DEBUG",
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

    // 启动 Server，开启 DebugChain
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 验证 Server 状态
    server::ServerStatus status;
    server.get_status(&status);
    EXPECT_EQ(status.status, server::SERVER_STATUS_RUNNING);

    // 使用 SimClient 发送带延迟调试头的请求
    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "POST", "/api/debug",
        {}, "", 0, 2000  // force_error_code=0, response_timeout_ms=2000
    );

    // 运行 SimClient（X-Debug-Response-Timeout-Ms 会自动添加）
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

// IT-006: DebugChain 强制错误码调测点
TEST_F(IntegrationTest, IT006_DebugChainForceErrorCode) {
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
            "enabled": true,
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

    // 启动 Server，开启 DebugChain
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 使用 SimClient 发送带强制错误码调试头的请求
    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "POST", "/api/debug",
        {}, "", 500, 5000  // force_error_code=500
    );

    // 运行 SimClient（X-Debug-Force-Error-Code 会自动添加）
    SimClientResult result = runner.run_scenario(client_config);

    if (!result.output.empty()) {
        std::cout << "[SimClient Output]\n" << result.output << std::endl;
    }

    // 停止 Server
    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// IT-007: DebugChain 强制断开调测点
TEST_F(IntegrationTest, IT007_DebugChainForceDisconnect) {
    uint16_t port = get_test_port();
    std::string config_json = R"({
        "listens": [
            {"ip": "127.0.0.1", "port": )" + std::to_string(port) + R"(, "enabled": true}
        ],
        "certificates": {
            "cert_path": "",
            "key_path": "",
            "ca_path": "",
            "cipher_suite": "TLS_AES_256_GCM_SHA384"
        },
        "debug": {
            "enabled": true,
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

    // 启动 Server，开启 DebugChain
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 停止 Server
    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// IT-023: DebugChain - 延迟响应 + 错误码叠加
TEST_F(IntegrationTest, IT023_DebugChainDelayAndErrorCombined) {
    uint16_t port = get_test_port();
    std::string config_json = R"({
        "listens": [
            {"ip": "127.0.0.1", "port": )" + std::to_string(port) + R"(, "enabled": true}
        ],
        "certificates": {
            "cert_path": "",
            "key_path": "",
            "ca_path": "",
            "cipher_suite": "TLS_AES_256_GCM_SHA384"
        },
        "debug": {
            "enabled": true,
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
    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// IT-024: DebugChain - 延迟响应 + 强制断开叠加
TEST_F(IntegrationTest, IT024_DebugChainDelayAndDisconnectCombined) {
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

// IT-025: DebugChain - 错误码 + 强制断开叠加
TEST_F(IntegrationTest, IT025_DebugChainErrorAndDisconnectCombined) {
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

// IT-026: DebugChain - 延迟 + 错误码 + 强制断开三重叠加
TEST_F(IntegrationTest, IT026_DebugChainTripleCombined) {
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

// IT-027: DebugChain - 日志记录调测点
TEST_F(IntegrationTest, IT027_DebugChainLogging) {
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
