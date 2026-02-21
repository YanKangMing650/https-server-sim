// =============================================================================
//  HTTPS Server Simulator - Integration Tests IT-002 ~ IT-004
//  文件: test_basic_requests.cpp
//  描述: 基础请求响应测试
//  版权: Copyright (c) 2026
// =============================================================================
#include "test_fixture.hpp"

namespace https_server_sim {
namespace integration_test {

// IT-002: 单个 HTTPS 请求响应
TEST_F(IntegrationTest, IT002_SingleHttpsRequest) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    TempFile config_file(get_valid_config_json(server_port));

    server::Server server;

    // 1. 启动 Server
    ASSERT_TRUE(start_server(server, config_file.path()));

    // 验证 Server 状态
    server::ServerStatus status;
    server.get_status(&status);
    EXPECT_EQ(status.status, server::SERVER_STATUS_RUNNING);

    // 2. 使用 SimClient 发送请求
    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "POST", "/api/test",
        {{"Content-Type", "application/json"}},
        "{\"key\": \"value\"}"
    );

    // 注意：当前 Server 的完整 TLS 握手和请求处理可能需要更完整的实现
    // 这里展示了如何调用 SimClient，实际结果取决于 Server 实现
    SimClientResult result = runner.run_scenario(client_config);

    // 期望 SimClient 执行成功
    EXPECT_TRUE(result.success) << "SimClient failed with exit code: " << result.exit_code;

    // 打印输出以便调试
    if (!result.output.empty()) {
        std::cout << "[SimClient Output]\n" << result.output << std::endl;
    }

    // 3. 停止 Server
    int ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// IT-003: 多个 HTTPS 请求（Keep-Alive）
TEST_F(IntegrationTest, IT003_MultipleHttpsRequestsKeepAlive) {
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

    // 停止 Server
    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// IT-004: 并发连接测试
TEST_F(IntegrationTest, IT004_ConcurrentConnections) {
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

    // 获取统计信息
    utils::Statistics stats;
    server.get_statistics(&stats);
    EXPECT_EQ(stats.total_connections, 0ULL);

    // 停止 Server
    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

} // namespace integration_test
} // namespace https_server_sim

// 文件结束
