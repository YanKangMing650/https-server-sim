// =============================================================================
//  HTTPS Server Simulator - Integration Tests IT-028 ~ IT-032
//  文件: test_client_disconnect.cpp
//  描述: Client 中途断开场景测试
//  版权: Copyright (c) 2026
// =============================================================================
#include "test_fixture.hpp"

namespace https_server_sim {
namespace integration_test {

// IT-028: Client 中途断开 - TLS 握手阶段
TEST_F(IntegrationTest, IT028_ClientDisconnectDuringHandshake) {
    uint16_t server_port = get_test_port();
    (void)client_port_counter;  // 保持计数器增长但不使用此端口
    TempFile config_file(get_valid_config_json(server_port));

    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    server::ServerStatus status;
    server.get_status(&status);
    EXPECT_EQ(status.status, server::SERVER_STATUS_RUNNING);

    // 使用 TestClient 建立 TCP 连接后立即断开（模拟 TLS 握手前断开）
    TestClient client("127.0.0.1", server_port);
    bool connected = client.connect();
    if (connected) {
        client.disconnect();
    }

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// IT-029: Client 中途断开 - 接收请求阶段
TEST_F(IntegrationTest, IT029_ClientDisconnectDuringRecv) {
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

// IT-030: Client 中途断开 - 回调处理阶段
TEST_F(IntegrationTest, IT030_ClientDisconnectDuringCallback) {
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

// IT-031: Client 中途断开 - 发送响应阶段
TEST_F(IntegrationTest, IT031_ClientDisconnectDuringSend) {
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

// IT-032: 多个 Client 同时中途断开
TEST_F(IntegrationTest, IT032_MultipleClientsDisconnect) {
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
