// Config模块单元测试

#include <gtest/gtest.h>
#include "config/config.hpp"
#include <fstream>
#include <cstdio>

namespace https_server_sim {
namespace config {

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.reset();
    }

    void TearDown() override {
    }

    Config config_;
};

// 测试用例: 加载完整JSON配置字符串
TEST_F(ConfigTest, LoadFromFullJsonString) {
    const std::string json_str = R"({
        "listens": [
            {"ip": "0.0.0.0", "port": 8443, "enabled": true, "backlog": 128}
        ],
        "certificates": {
            "cert_path": "certs/server.crt",
            "key_path": "certs/server.key",
            "ca_path": "certs/ca.crt",
            "sm2_cert_path": "certs/sm2/server.crt",
            "sm2_key_path": "certs/sm2/server.key",
            "sm2_sign_cert_path": "certs/sm2/sign.crt",
            "sm2_sign_key_path": "certs/sm2/sign.key",
            "cipher_suite": "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256"
        },
        "debug": {
            "enabled": false,
            "debug_points": [
                {"server_port": 8443, "point_name": "tls_handshake", "action": "delay", "delay_ms": 100, "error_code": 0, "probability": 100}
            ]
        },
        "callbacks": {
            "callbacks_dir": "callbacks",
            "callbacks": [
                {"server_port": 8443, "script_path": "callback.lua"}
            ]
        },
        "logging": {
            "level": "INFO",
            "file": "server.log",
            "console_output": true
        },
        "http2": {
            "enabled": true,
            "allow_h2c": false
        }
    })";

    int ret = config_.load_from_string(json_str);
    EXPECT_EQ(ret, 0);

    // 验证listens
    const auto& listens = config_.get_listens();
    ASSERT_EQ(listens.size(), static_cast<size_t>(1));
    EXPECT_EQ(listens[0].ip, "0.0.0.0");
    EXPECT_EQ(listens[0].port, static_cast<uint16_t>(8443));
    EXPECT_EQ(listens[0].enabled, true);
    EXPECT_EQ(listens[0].backlog, static_cast<uint32_t>(128));

    // 验证certificates
    const auto& certs = config_.get_certificates();
    EXPECT_EQ(certs.cert_path, "certs/server.crt");
    EXPECT_EQ(certs.key_path, "certs/server.key");
    EXPECT_EQ(certs.ca_path, "certs/ca.crt");
    EXPECT_EQ(certs.sm2_cert_path, "certs/sm2/server.crt");
    EXPECT_EQ(certs.sm2_key_path, "certs/sm2/server.key");
    EXPECT_EQ(certs.sm2_sign_cert_path, "certs/sm2/sign.crt");
    EXPECT_EQ(certs.sm2_sign_key_path, "certs/sm2/sign.key");
    EXPECT_EQ(certs.cipher_suite, "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256");

    // 验证debug
    const auto& debug = config_.get_debug();
    EXPECT_EQ(debug.enabled, false);
    ASSERT_EQ(debug.debug_points.size(), static_cast<size_t>(1));
    EXPECT_EQ(debug.debug_points[0].server_port, static_cast<uint16_t>(8443));
    EXPECT_EQ(debug.debug_points[0].point_name, "tls_handshake");
    EXPECT_EQ(debug.debug_points[0].action, "delay");
    EXPECT_EQ(debug.debug_points[0].delay_ms, static_cast<uint32_t>(100));
    EXPECT_EQ(debug.debug_points[0].error_code, static_cast<int32_t>(0));
    EXPECT_EQ(debug.debug_points[0].probability, static_cast<uint32_t>(100));

    // 验证callbacks
    const auto& callbacks = config_.get_callbacks();
    EXPECT_EQ(callbacks.callbacks_dir, "callbacks");
    ASSERT_EQ(callbacks.callbacks.size(), static_cast<size_t>(1));
    EXPECT_EQ(callbacks.callbacks[0].server_port, static_cast<uint16_t>(8443));
    EXPECT_EQ(callbacks.callbacks[0].script_path, "callback.lua");
    EXPECT_EQ(config_.get_callbacks_dir(), "callbacks");

    // 验证logging
    const auto& logging = config_.get_logging();
    EXPECT_EQ(logging.level, "INFO");
    EXPECT_EQ(logging.file, "server.log");
    EXPECT_EQ(logging.console_output, true);

    // 验证http2
    const auto& http2 = config_.get_http2();
    EXPECT_EQ(http2.enabled, true);
    EXPECT_EQ(http2.allow_h2c, false);
}

// 测试用例: 加载部分JSON配置（缺失部分字段）
TEST_F(ConfigTest, LoadFromPartialJsonString) {
    const std::string json_str = R"({
        "listens": [
            {"ip": "127.0.0.1", "port": 9000}
        ],
        "logging": {
            "level": "DEBUG"
        }
    })";

    int ret = config_.load_from_string(json_str);
    EXPECT_EQ(ret, 0);

    const auto& listens = config_.get_listens();
    ASSERT_EQ(listens.size(), static_cast<size_t>(1));
    EXPECT_EQ(listens[0].ip, "127.0.0.1");
    EXPECT_EQ(listens[0].port, static_cast<uint16_t>(9000));
    EXPECT_EQ(listens[0].enabled, true);
    EXPECT_EQ(listens[0].backlog, static_cast<uint32_t>(128));

    const auto& logging = config_.get_logging();
    EXPECT_EQ(logging.level, "DEBUG");
}

// 测试用例: 加载无效JSON字符串
TEST_F(ConfigTest, LoadFromInvalidJsonString) {
    const std::string json_str = "{ invalid json }";
    int ret = config_.load_from_string(json_str);
    EXPECT_EQ(ret, -1);
}

// 测试用例: 加载空JSON字符串
TEST_F(ConfigTest, LoadFromEmptyJsonString) {
    const std::string json_str = "{}";
    int ret = config_.load_from_string(json_str);
    EXPECT_EQ(ret, 0);
}

// 测试用例: 从不存在的文件加载
TEST_F(ConfigTest, LoadFromNonExistentFile) {
    int ret = config_.load_from_file("/path/does/not/exist/config.json");
    EXPECT_EQ(ret, -1);
}

// 测试用例: 从临时文件加载
TEST_F(ConfigTest, LoadFromValidFile) {
    const std::string temp_filename = "/tmp/test_config.json";
    const std::string json_content = R"({
        "listens": [
            {"ip": "192.168.1.1", "port": 8080, "enabled": false, "backlog": 64}
        ]
    })";

    // 创建临时文件
    std::ofstream file(temp_filename);
    ASSERT_TRUE(file.is_open());
    file << json_content;
    file.close();

    // 加载配置
    int ret = config_.load_from_file(temp_filename);
    EXPECT_EQ(ret, 0);

    // 验证
    const auto& listens = config_.get_listens();
    ASSERT_EQ(listens.size(), static_cast<size_t>(1));
    EXPECT_EQ(listens[0].ip, "192.168.1.1");
    EXPECT_EQ(listens[0].port, static_cast<uint16_t>(8080));
    EXPECT_EQ(listens[0].enabled, false);
    EXPECT_EQ(listens[0].backlog, static_cast<uint32_t>(64));

    // 清理临时文件
    std::remove(temp_filename.c_str());
}

// 测试用例: 配置验证 - 空listens
TEST_F(ConfigTest, ValidateEmptyListens) {
    std::vector<ListenConfig> empty_listens;
    config_.set_listens(empty_listens);
    int ret = config_.validate();
    EXPECT_EQ(ret, -1);
}

// 测试用例: 配置验证 - 有效配置
TEST_F(ConfigTest, ValidateValidConfig) {
    int ret = config_.validate();
    EXPECT_EQ(ret, 0);
}

// 测试用例: 设置和获取配置
TEST_F(ConfigTest, SetAndGetConfig) {
    ListenConfig listen;
    listen.ip = "10.0.0.1";
    listen.port = 12345;
    listen.enabled = true;
    listen.backlog = 256;
    std::vector<ListenConfig> listens = {listen};
    config_.set_listens(listens);

    const auto& result = config_.get_listens();
    ASSERT_EQ(result.size(), static_cast<size_t>(1));
    EXPECT_EQ(result[0].ip, "10.0.0.1");
    EXPECT_EQ(result[0].port, static_cast<uint16_t>(12345));
    EXPECT_EQ(result[0].enabled, true);
    EXPECT_EQ(result[0].backlog, static_cast<uint32_t>(256));
}

// 测试用例: 重置配置
TEST_F(ConfigTest, ResetConfig) {
    const std::string json_str = R"({
        "listens": [{"ip": "1.2.3.4", "port": 5678}],
        "logging": {"level": "ERROR"}
    })";
    config_.load_from_string(json_str);

    config_.reset();

    const auto& listens = config_.get_listens();
    ASSERT_EQ(listens.size(), static_cast<size_t>(1));
    EXPECT_EQ(listens[0].ip, "0.0.0.0");
    EXPECT_EQ(listens[0].port, static_cast<uint16_t>(8443));
    EXPECT_EQ(config_.get_logging().level, "INFO");
}

// 测试用例: 多listens配置
TEST_F(ConfigTest, MultipleListens) {
    const std::string json_str = R"({
        "listens": [
            {"ip": "0.0.0.0", "port": 8443, "enabled": true},
            {"ip": "127.0.0.1", "port": 9443, "enabled": false}
        ]
    })";

    int ret = config_.load_from_string(json_str);
    EXPECT_EQ(ret, 0);

    const auto& listens = config_.get_listens();
    ASSERT_EQ(listens.size(), static_cast<size_t>(2));
    EXPECT_EQ(listens[0].ip, "0.0.0.0");
    EXPECT_EQ(listens[0].port, static_cast<uint16_t>(8443));
    EXPECT_EQ(listens[0].enabled, true);
    EXPECT_EQ(listens[1].ip, "127.0.0.1");
    EXPECT_EQ(listens[1].port, static_cast<uint16_t>(9443));
    EXPECT_EQ(listens[1].enabled, false);
}

// 测试用例: 多debug_points配置
TEST_F(ConfigTest, MultipleDebugPoints) {
    const std::string json_str = R"({
        "debug": {
            "enabled": true,
            "debug_points": [
                {"server_port": 8443, "point_name": "tls_handshake", "action": "delay", "delay_ms": 100},
                {"server_port": 9443, "point_name": "request", "action": "error", "error_code": 500}
            ]
        }
    })";

    int ret = config_.load_from_string(json_str);
    EXPECT_EQ(ret, 0);

    const auto& debug = config_.get_debug();
    EXPECT_EQ(debug.enabled, true);
    ASSERT_EQ(debug.debug_points.size(), static_cast<size_t>(2));
    EXPECT_EQ(debug.debug_points[0].server_port, static_cast<uint16_t>(8443));
    EXPECT_EQ(debug.debug_points[0].action, "delay");
    EXPECT_EQ(debug.debug_points[0].delay_ms, static_cast<uint32_t>(100));
    EXPECT_EQ(debug.debug_points[1].server_port, static_cast<uint16_t>(9443));
    EXPECT_EQ(debug.debug_points[1].action, "error");
    EXPECT_EQ(debug.debug_points[1].error_code, static_cast<int32_t>(500));
}

// 测试用例: callbacks_dir设置
TEST_F(ConfigTest, CallbacksDir) {
    config_.set_callbacks_dir("custom_callbacks");
    EXPECT_EQ(config_.get_callbacks_dir(), "custom_callbacks");
}

// 测试用例: 类型错误处理
TEST_F(ConfigTest, TypeMismatchHandling) {
    const std::string json_str = R"({
        "listens": "not an array"
    })";
    int ret = config_.load_from_string(json_str);
    EXPECT_EQ(ret, 0);
}

} // namespace config
} // namespace https_server_sim
