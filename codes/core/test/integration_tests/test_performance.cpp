// =============================================================================
//  HTTPS Server Simulator - Performance Tests PERF-001 ~ PERF-003
//  文件: test_performance.cpp
//  描述: 性能测试用例
//  版权: Copyright (c) 2026
// =============================================================================
#include "test_fixture.hpp"
#include <vector>

namespace https_server_sim {
namespace integration_test {

// PERF-001: 性能基准测试 - 延迟
TEST_F(IntegrationTest, PERF001_PerformanceLatency) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    TempFile config_file(get_valid_config_json(server_port));

    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 验证 Server 运行正常
    server::ServerStatus status;
    server.get_status(&status);
    EXPECT_EQ(status.status, server::SERVER_STATUS_RUNNING);

    // 发送多次请求来测量延迟
    SimClientRunner runner(test_dir_);
    const int iterations = 5;
    for (int i = 0; i < iterations; ++i) {
        std::string client_config = SimClientRunner::build_single_request_config(
            client_port + i, server_port, "GET", "/api/latency",
            {}, ""
        );
        SimClientResult result = runner.run_scenario(client_config);
        // 期望每次 SimClient 执行都成功
        EXPECT_TRUE(result.success) << "SimClient iteration " << i << " failed with exit code: " << result.exit_code;
        if (!result.output.empty()) {
            std::cout << "[Iteration " << i << " Output]\n" << result.output << std::endl;
        }
    }

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// PERF-002: 性能基准测试 - 吞吐量（RPS）
TEST_F(IntegrationTest, PERF002_PerformanceThroughput) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    TempFile config_file(get_valid_config_json(server_port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 准备多个请求配置用于吞吐量测试
    SimClientRunner runner(test_dir_);
    std::vector<std::string> client_configs;
    for (int i = 0; i < 10; ++i) {
        client_configs.push_back(SimClientRunner::build_single_request_config(
            client_port + i, server_port, "POST", "/api/throughput",
            {{"Content-Type", "application/json"}},
            "{\"seq\":" + std::to_string(i) + "}"
        ));
    }

    // 顺序执行请求
    for (size_t i = 0; i < client_configs.size(); ++i) {
        SimClientResult result = runner.run_scenario(client_configs[i]);
        // 期望每次 SimClient 执行都成功
        EXPECT_TRUE(result.success) << "SimClient request " << i << " failed with exit code: " << result.exit_code;
    }

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// PERF-003: 性能测试 - 大报文处理
TEST_F(IntegrationTest, PERF003_PerformanceLargePayload) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    TempFile config_file(get_valid_config_json(server_port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 创建较大的请求体
    std::string large_body(4096, 'x');  // 4KB 数据
    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "POST", "/api/large_payload",
        {{"Content-Type", "application/octet-stream"}},
        large_body
    );

    SimClientResult result = runner.run_scenario(client_config);
    // 期望 SimClient 执行成功
    EXPECT_TRUE(result.success) << "SimClient failed with exit code: " << result.exit_code;
    if (!result.output.empty()) {
        std::cout << "[Large Payload Output]\n" << result.output << std::endl;
    }

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

} // namespace integration_test
} // namespace https_server_sim

// 文件结束
