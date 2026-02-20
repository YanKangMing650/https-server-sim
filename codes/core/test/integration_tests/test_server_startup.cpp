// =============================================================================
//  HTTPS Server Simulator - Integration Test
//  文件: test_server_startup.cpp
//  描述: Server启动流程集成测试 - 验证Server + Config + MsgCenter模块协作
//  版权: Copyright (c) 2026
// =============================================================================

#include <gtest/gtest.h>
#include <fstream>
#include <thread>
#include <chrono>
#include <atomic>
#include "server/server.hpp"
#include "config/config.hpp"
#include "msg_center/msg_center.hpp"
#include "utils/logger.hpp"

namespace https_server_sim {
namespace integration_test {

// ==================== 测试辅助函数 ====================

static std::atomic<uint64_t> temp_file_counter{0};

class TempFile {
public:
    explicit TempFile(const std::string& content) {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()).count();
        uint64_t counter = temp_file_counter.fetch_add(1);
        path_ = "/tmp/test_integration_" +
               std::to_string(timestamp) + "_" +
               std::to_string(counter) + ".json";
        std::ofstream file(path_);
        file << content;
        file.close();
    }

    ~TempFile() {
        if (!path_.empty()) {
            unlink(path_.c_str());
        }
    }

    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;
    TempFile(TempFile&& other) noexcept : path_(std::move(other.path_)) {
        other.path_.clear();
    }

    const std::string& path() const { return path_; }

private:
    std::string path_;
};

std::string get_valid_config_json() {
    return R"({
        "listens": [
            {"ip": "127.0.0.1", "port": 19443, "enabled": true}
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
}

// ==================== 集成测试用例 ====================

// Integration_UseCase001: Server完整启动流程
// 验证: Server -> Config -> MsgCenter 初始化顺序和协作
TEST(IntegrationTest, UseCase001_ServerFullStartup) {
    TempFile config_file(get_valid_config_json());

    // 阶段1: 初始化Server
    Server server;
    int ret = server.init(config_file.path());
    EXPECT_EQ(ret, 0) << "Server::init() should succeed";

    // 验证状态: STOPPED
    ServerStatus status;
    server.get_status(&status);
    EXPECT_EQ(status.status, SERVER_STATUS_STOPPED);

    // 阶段2: 启动Server
    ret = server.start();
    EXPECT_EQ(ret, 0) << "Server::start() should succeed";

    // 验证状态: RUNNING
    server.get_status(&status);
    EXPECT_EQ(status.status, SERVER_STATUS_RUNNING);

    // 阶段3: 停止Server
    ret = server.stop();
    EXPECT_EQ(ret, 0) << "Server::stop() should succeed";

    // 验证状态: STOPPED
    server.get_status(&status);
    EXPECT_EQ(status.status, SERVER_STATUS_STOPPED);

    // 阶段4: 清理资源
    server.cleanup();

    // 验证最终状态
    server.get_status(&status);
    EXPECT_EQ(status.status, SERVER_STATUS_STOPPED);
}

// Integration_UseCase002: 配置加载验证
// 验证: Config模块正确加载配置并传递给Server
TEST(IntegrationTest, UseCase002_ConfigLoading) {
    TempFile config_file(get_valid_config_json());

    // 直接测试Config模块
    config::Config config;
    int ret = config.load_from_file(config_file.path());
    EXPECT_EQ(ret, 0);

    ret = config.validate();
    EXPECT_EQ(ret, 0);

    // 验证配置项
    const auto& listens = config.get_listens();
    EXPECT_GE(listens.size(), 1UL);

    // 验证Server能使用这个配置
    Server server;
    ret = server.init(config_file.path());
    EXPECT_EQ(ret, 0);

    server.cleanup();
}

// Integration_UseCase003: MsgCenter启动和停止
// 验证: MsgCenter能独立启动和停止，线程正确管理
TEST(IntegrationTest, UseCase003_MsgCenterStartStop) {
    // 创建MsgCenter
    MsgCenter msg_center(2, 2);

    // 启动
    int ret = msg_center.start();
    EXPECT_EQ(ret, 0) << "MsgCenter::start() should succeed";

    // 等待一小段时间让线程启动
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 停止
    msg_center.stop();

    SUCCEED() << "MsgCenter started and stopped successfully";
}

// Integration_UseCase004: 快速启动-停止循环
// 验证: Server能承受多次快速启动停止
TEST(IntegrationTest, UseCase004_RapidStartStopCycle) {
    TempFile config_file(get_valid_config_json());

    Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);

    // 执行3次启动-停止循环
    for (int i = 0; i < 3; i++) {
        SCOPED_TRACE("Cycle " + std::to_string(i));

        ret = server.start();
        EXPECT_EQ(ret, 0);

        // 短暂运行
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        ret = server.stop();
        EXPECT_EQ(ret, 0);
    }

    server.cleanup();
}

// Integration_UseCase005: 状态查询不阻塞
// 验证: 在Server启动/停止过程中，get_statistics()不被阻塞
TEST(IntegrationTest, UseCase005_StatusQueryNonBlocking) {
    TempFile config_file(get_valid_config_json());

    Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);

    // 在单独线程中启动Server
    std::atomic<bool> started{false};
    std::thread start_thread([&]() {
        ret = server.start();
        (void)ret;
        started = true;
    });

    // 同时查询状态 - 不应该被阻塞
    auto start_time = std::chrono::steady_clock::now();
    for (int i = 0; i < 10; i++) {
        ServerStatus status;
        server.get_status(&status);
        // 只要不崩溃就好
    }
    auto end_time = std::chrono::steady_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    EXPECT_LT(duration.count(), 500LL)
        << "Status queries should complete quickly";

    // 等待启动完成
    start_thread.join();

    // 停止
    server.stop();
    server.cleanup();
}

} // namespace integration_test
} // namespace https_server_sim

// 文件结束
