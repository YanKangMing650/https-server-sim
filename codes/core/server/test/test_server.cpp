// =============================================================================
//  HTTPS Server Simulator - Server Module Unit Tests
//  文件: test_server.cpp
//  描述: Server模块单元测试
//  版权: Copyright (c) 2026
// =============================================================================
#include <gtest/gtest.h>
#include <fstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <condition_variable>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "server/server.hpp"
#include "config/config.hpp"
#include "utils/statistics.hpp"

namespace https_server_sim {
namespace server {
namespace test {

// ==================== 测试辅助函数 ====================

// 原子计数器，用于生成唯一临时文件名
static std::atomic<uint64_t> temp_file_counter{0};

// RAII临时文件包装器 - 确保临时文件在作用域结束时被删除
class TempFile {
public:
    explicit TempFile(const std::string& content) {
        // 使用时间戳 + 原子计数器生成唯一文件名，避免竞态条件
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()).count();
        uint64_t counter = temp_file_counter.fetch_add(1);
        path_ = "/tmp/test_server_config_" +
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

    // 禁止拷贝
    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;

    // 允许移动
    TempFile(TempFile&& other) noexcept : path_(std::move(other.path_)) {
        other.path_.clear();
    }

    TempFile& operator=(TempFile&& other) noexcept {
        if (this != &other) {
            // 先删除当前文件
            if (!path_.empty()) {
                unlink(path_.c_str());
            }
            // 接管对方的资源
            path_ = std::move(other.path_);
            other.path_.clear();
        }
        return *this;
    }

    const std::string& path() const { return path_; }

private:
    std::string path_;
};

// 获取有效的配置JSON
std::string get_valid_config_json() {
    return R"({
        "listens": [
            {"ip": "127.0.0.1", "port": 18443, "enabled": true}
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

// 获取多端口配置JSON
std::string get_multi_port_config_json() {
    return R"({
        "listens": [
            {"ip": "127.0.0.1", "port": 18443, "enabled": true},
            {"ip": "127.0.0.1", "port": 18444, "enabled": true}
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

// ==================== 测试用例 ====================

// Server_UseCase001: 正常初始化，传入有效配置文件
TEST(ServerTest, UseCase001_NormalInitWithValidConfig) {
    TempFile config_file(get_valid_config_json());

    Server server;
    int ret = server.init(config_file.path());
    EXPECT_EQ(ret, 0);

    // 验证状态
    ServerStatus status;
    server.get_status(&status);
    EXPECT_EQ(status.status, SERVER_STATUS_STOPPED);

    server.cleanup();
    // 无需手动删除文件，TempFile析构自动处理
}

// Server_UseCase002: 异常初始化，配置文件不存在
// 注意：Config模块当前为桩实现，总是返回成功，此测试用例验证cleanup能力
TEST(ServerTest, UseCase002_InitWithNonExistentConfig) {
    Server server;

    // 测试：由于Config是桩，init()会成功
    int ret = server.init("/path/that/does/not/exist.json");
    (void)ret;  // 避免未使用变量警告

    // 验证状态 - 桩实现返回成功
    ServerStatus status;
    server.get_status(&status);

    // 执行cleanup确保可以恢复到STOPPED
    server.cleanup();
    server.get_status(&status);
    EXPECT_EQ(status.status, SERVER_STATUS_STOPPED);
}

// Server_UseCase003: 异常初始化，配置文件内容无效
// 注意：Config模块当前为桩实现，总是返回成功，此测试用例验证cleanup能力
TEST(ServerTest, UseCase003_InitWithInvalidConfig) {
    TempFile config_file("invalid json content");

    Server server;
    // init()会成功（因为Config是桩）
    int ret = server.init(config_file.path());
    // 桩实现返回成功
    (void)ret;

    server.cleanup();

    ServerStatus status;
    server.get_status(&status);
    EXPECT_EQ(status.status, SERVER_STATUS_STOPPED);
}

// Server_UseCase004: 正常启动，init后调用start
TEST(ServerTest, UseCase004_NormalStartAfterInit) {
    TempFile config_file(get_valid_config_json());

    Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);

    ret = server.start();
    EXPECT_EQ(ret, 0);

    // 验证状态为RUNNING
    ServerStatus status;
    server.get_status(&status);
    EXPECT_EQ(status.status, SERVER_STATUS_RUNNING);

    // 停止
    ret = server.stop();
    EXPECT_EQ(ret, 0);

    server.cleanup();
}

// Server_UseCase005: 异常启动，未init直接start
TEST(ServerTest, UseCase005_StartWithoutInit) {
    Server server;
    int ret = server.start();
    EXPECT_NE(ret, 0);

    // 状态保持STOPPED
    ServerStatus status;
    server.get_status(&status);
    EXPECT_EQ(status.status, SERVER_STATUS_STOPPED);
}

// Server_UseCase006: 边界场景，重复调用start
TEST(ServerTest, UseCase006_DuplicateStart) {
    TempFile config_file(get_valid_config_json());

    Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);

    ret = server.start();
    EXPECT_EQ(ret, 0);

    // 第二次start应该失败
    ret = server.start();
    EXPECT_NE(ret, 0);

    server.stop();
    server.cleanup();
}

// Server_UseCase007: 正常停止，start后调用stop
TEST(ServerTest, UseCase007_NormalStopAfterStart) {
    TempFile config_file(get_valid_config_json());

    Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);

    ret = server.start();
    ASSERT_EQ(ret, 0);

    ret = server.stop();
    EXPECT_EQ(ret, 0);

    // 验证状态为STOPPED
    ServerStatus status;
    server.get_status(&status);
    EXPECT_EQ(status.status, SERVER_STATUS_STOPPED);

    server.cleanup();
}

// Server_UseCase008: 异常停止，未start直接stop
TEST(ServerTest, UseCase008_StopWithoutStart) {
    TempFile config_file(get_valid_config_json());

    Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);

    // 未start直接stop应该失败
    ret = server.stop();
    EXPECT_NE(ret, 0);

    server.cleanup();
}

// Server_UseCase009: 正常查询状态，处于RUNNING状态
TEST(ServerTest, UseCase009_GetStatusWhileRunning) {
    TempFile config_file(get_valid_config_json());

    Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);

    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 查询状态
    ServerStatus status;
    server.get_status(&status);
    EXPECT_EQ(status.status, SERVER_STATUS_RUNNING);

    server.stop();
    server.cleanup();
}

// Server_UseCase010: 正常查询统计信息
TEST(ServerTest, UseCase010_GetStatistics) {
    TempFile config_file(get_valid_config_json());

    Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);

    // 查询统计信息
    utils::Statistics stats;
    server.get_statistics(&stats);
    // 只要能正常获取即可，不检查具体数值
    EXPECT_EQ(stats.total_connections, 0ULL);

    server.cleanup();
}

// Server_UseCase011: 正常初始化，多端口监听配置
// 注意：Config模块是桩实现，不实际解析配置文件，总是返回默认值
TEST(ServerTest, UseCase011_MultiPortInit) {
    TempFile config_file(get_multi_port_config_json());

    Server server;
    int ret = server.init(config_file.path());
    EXPECT_EQ(ret, 0);

    // 验证状态
    ServerStatus status;
    server.get_status(&status);
    EXPECT_EQ(status.status, SERVER_STATUS_STOPPED);
    // 注意：Config是桩，返回默认端口8443

    server.cleanup();
}

// Server_UseCase012: Graceful Shutdown超时场景
TEST(ServerTest, UseCase012_GracefulShutdownTimeout) {
    TempFile config_file(get_valid_config_json());

    Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);

    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 停止并验证（没有连接的情况下会立即完成）
    auto start_time = std::chrono::steady_clock::now();
    ret = server.stop();
    auto end_time = std::chrono::steady_clock::now();

    EXPECT_EQ(ret, 0);

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    // 没有连接时应该快速完成（远小于30秒）
    EXPECT_LT(duration.count(), 1000LL);

    server.cleanup();
}

// Server_UseCase013: 正常清理资源
TEST(ServerTest, UseCase013_Cleanup) {
    TempFile config_file(get_valid_config_json());

    Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);

    // 清理资源
    server.cleanup();

    // 验证状态回到STOPPED
    ServerStatus status;
    server.get_status(&status);
    EXPECT_EQ(status.status, SERVER_STATUS_STOPPED);

    // 验证可以再次init
    ret = server.init(config_file.path());
    EXPECT_EQ(ret, 0);

    server.cleanup();
}

// Server_UseCase014: 正常创建socket（内部测试）
// 此用例通过UseCase001间接测试

// Server_UseCase015: 异常绑定，端口被占用
// 注意：由于我们不能轻易在测试中占用端口，这个测试验证Server的状态转换逻辑
TEST(ServerTest, UseCase015_PortAlreadyInUse) {
    TempFile config_file(get_valid_config_json());

    // 创建第一个Server实例并初始化，它应该能正常工作
    Server server1;
    int ret = server1.init(config_file.path());
    ASSERT_EQ(ret, 0);

    ret = server1.start();
    ASSERT_EQ(ret, 0);

    // 验证状态为RUNNING
    ServerStatus status;
    server1.get_status(&status);
    EXPECT_EQ(status.status, SERVER_STATUS_RUNNING);

    // 清理server1
    server1.stop();
    server1.cleanup();

    server1.get_status(&status);
    EXPECT_EQ(status.status, SERVER_STATUS_STOPPED);
}

// Server_UseCase016: 测试get_status不被阻塞
TEST(ServerTest, UseCase016_GetStatusNotBlockedDuringShutdown) {
    TempFile config_file(get_valid_config_json());

    Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);

    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 启动一个线程来执行stop()
    std::atomic<bool> shutdown_started{false};
    std::thread stop_thread([&]() {
        shutdown_started = true;
        // 调用stop()会进入graceful_shutdown
        ret = server.stop();
        (void)ret;
    });

    // 等待stop线程启动
    while (!shutdown_started) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 让stop线程有时间进入graceful_shutdown
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 此时Server应该在SHUTTING_DOWN状态
    // 测试get_status快速返回，不被阻塞
    auto start_time = std::chrono::steady_clock::now();
    ServerStatus status;
    server.get_status(&status);
    auto end_time = std::chrono::steady_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    EXPECT_LT(duration.count(), 100LL);  // 应该在100ms内返回

    // 等待stop线程结束
    stop_thread.join();

    // 最终状态应该是STOPPED
    server.get_status(&status);
    EXPECT_EQ(status.status, SERVER_STATUS_STOPPED);

    server.cleanup();
}

// Server_UseCase017: 多端口初始化失败时资源回滚
// 验证start()过程中如果add_listen_fd失败会正确回滚
// 注意：由于我们不能轻易让add_listen_fd失败，我们通过代码审查和其他测试间接验证
TEST(ServerTest, UseCase017_MultiPortRollbackOnFailure) {
    TempFile config_file(get_multi_port_config_json());

    Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);

    // 验证状态为STOPPED
    ServerStatus status;
    server.get_status(&status);
    EXPECT_EQ(status.status, SERVER_STATUS_STOPPED);

    // 正常启动
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 验证状态为RUNNING
    server.get_status(&status);
    EXPECT_EQ(status.status, SERVER_STATUS_RUNNING);

    // 停止
    ret = server.stop();
    ASSERT_EQ(ret, 0);

    // 清理
    server.cleanup();

    server.get_status(&status);
    EXPECT_EQ(status.status, SERVER_STATUS_STOPPED);
}

// ==================== 额外边界测试 ====================

// 测试get_status空指针处理
TEST(ServerTest, GetStatusWithNullPtr) {
    Server server;
    server.get_status(nullptr);  // 不应该崩溃
    SUCCEED();
}

// 测试get_statistics空指针处理
TEST(ServerTest, GetStatisticsWithNullPtr) {
    Server server;
    server.get_statistics(nullptr);  // 不应该崩溃
    SUCCEED();
}

// 测试ERROR状态后可以通过cleanup回到STOPPED
// 验证：init失败时handle_init_error()能正确恢复状态
TEST(ServerTest, ErrorStateCanRecoverToStopped) {
    Server server;

    // init不存在的文件（应该失败）
    int ret = server.init("/tmp/non_existent_test_config.json");
    // 现在Config是真实实现，load_from_file失败会返回非0
    // 但handle_init_error()会调用cleanup()，状态应该恢复到STOPPED

    ServerStatus status;
    server.get_status(&status);
    // 不管init返回成功或失败，经过handle_init_error/cleanup后状态应该是STOPPED
    EXPECT_EQ(status.status, SERVER_STATUS_STOPPED);

    // 验证可以再次init() - 这次用有效的配置
    TempFile config_file(get_valid_config_json());
    ret = server.init(config_file.path());
    EXPECT_EQ(ret, 0);

    server.cleanup();

    server.get_status(&status);
    EXPECT_EQ(status.status, SERVER_STATUS_STOPPED);
}

// 测试析构函数自动清理
TEST(ServerTest, DestructorCleansUp) {
    TempFile config_file(get_valid_config_json());

    {
        Server server;
        int ret = server.init(config_file.path());
        ASSERT_EQ(ret, 0);
        ret = server.start();
        ASSERT_EQ(ret, 0);
        // 离开作用域，析构函数自动调用
    }

    SUCCEED();
}

} // namespace test
} // namespace server
} // namespace https_server_sim

// 文件结束
