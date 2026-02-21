// =============================================================================
//  HTTPS Server Simulator - Integration Tests IT-014 ~ IT-027, IT-051 ~ IT-060
//  文件: test_tls.cpp
//  描述: TLS 相关测试用例（含国密 SM2/SM3/SM4/SM9）
//  版权: Copyright (c) 2026
//
//  国密算法说明：
//    - SM2: 椭圆曲线公钥密码算法 (ECC-based)，用于数字签名、密钥交换和加密
//           对应国际标准: ECDSA / ECDH
//           密码套件中的位置: 密钥交换和认证部分
//
//    - SM3: 密码哈希算法，输出 256 位哈希值
//           对应国际标准: SHA-256
//           密码套件中的位置: 作为 PRF (伪随机函数) 和哈希算法
//
//    - SM4: 分组密码算法，分组长度 128 位，密钥长度 128 位
//           对应国际标准: AES-128
//           代码中又称: SMS4
//           密码套件中的位置: 对称加密部分
//
//    - SM9: 标识密码算法 (Identity-Based Cryptography)
//           特点: 使用用户标识作为公钥，无需证书
//           TLS 中的应用: 较少见，主要用于特定国密场景
//
//  国密密码套件 (在 tls_handler.cpp:759-762 定义):
//    - SM2-WITH-SMS4-SM3
//    - ECDHE-SM2-WITH-SMS4-SM3      (带前向安全性)
//    - SM2-WITH-SMS4-GCM-SM3        (GCM 模式)
//    - ECDHE-SM2-WITH-SMS4-GCM-SM3  (GCM 模式 + 前向安全性)
// =============================================================================
#include "test_fixture.hpp"

namespace https_server_sim {
namespace integration_test {

// IT-014: TLS 1.2 握手
TEST_F(IntegrationTest, IT014_Tls12Handshake) {
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
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 使用 SimClient 发送 TLS 1.2 请求
    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "GET", "/api/tls12",
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

// IT-015: TLS 1.3 握手
TEST_F(IntegrationTest, IT015_Tls13Handshake) {
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
        client_port, server_port, "GET", "/api/tls13",
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

// IT-016: TLS 证书验证 - 有效证书
TEST_F(IntegrationTest, IT016_TlsCertValidation_ValidCert) {
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
        client_port, server_port, "GET", "/api/valid_cert",
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

// IT-017: TLS 证书验证 - 无效证书
TEST_F(IntegrationTest, IT017_TlsCertValidation_InvalidCert) {
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

// IT-018: TLS 证书验证 - 无证书
TEST_F(IntegrationTest, IT018_TlsCertValidation_NoCert) {
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

// IT-019: TLS 密码套件 - AES-GCM
TEST_F(IntegrationTest, IT019_TlsCipherSuite_AesGcm) {
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
        client_port, server_port, "POST", "/api/aesgcm",
        {{"Content-Type", "application/json"}},
        "{\"cipher\": \"AES-GCM\"}"
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

// IT-020: TLS 密码套件 - ChaCha20-Poly1305
TEST_F(IntegrationTest, IT020_TlsCipherSuite_ChaCha20) {
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
        client_port, server_port, "POST", "/api/chacha20",
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

// IT-021: TLS 国密 SM2 单证书
TEST_F(IntegrationTest, IT021_TlsGMSSL_Sm2Single) {
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
        client_port, server_port, "GET", "/api/gmssl_sm2_single",
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

// IT-022: TLS 国密 SM2 双证书
TEST_F(IntegrationTest, IT022_TlsGMSSL_Sm2Dual) {
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
        client_port, server_port, "GET", "/api/gmssl_sm2_dual",
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

// IT-023: TLS 国密 SM3 哈希算法
//
// 【SM3 算法说明】
//   - 类型: 密码哈希算法 (Cryptographic Hash Function)
//   - 输出长度: 256 位 (32 字节)
//   - 对应国际标准: SHA-256
//   - 应用场景:
//     * 数字签名中的消息摘要
//     * HMAC 消息认证码
//     * TLS 中的 PRF (伪随机函数)
//     * 证书指纹计算
//
// 【在密码套件中的位置】
//   套件 "SM2-WITH-SMS4-SM3" 中:
//     - SM2:    密钥交换和认证
//     - SMS4:   对称加密 (SM4)
//     - SM3:    哈希算法和 PRF
//
TEST_F(IntegrationTest, IT023_TlsGMSSL_Sm3Hash) {
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
            "sm2_cert_path": "",
            "sm2_key_path": "",
            "sm2_sign_cert_path": "",
            "sm2_sign_key_path": "",
            "cipher_suite": "SM2-WITH-SMS4-SM3"
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

    // 使用 SM3 哈希算法的密码套件: SM2-WITH-SMS4-SM3
    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "POST", "/api/gmssl_sm3",
        {{"Content-Type", "application/json"}},
        "{\"hash\": \"SM3\", \"description\": \"国密哈希算法，输出256位摘要\"}"
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

// IT-024: TLS 国密 SM4 分组密码算法
//
// 【SM4 算法说明】
//   - 类型: 分组密码算法 (Block Cipher)
//   - 曾用名: SMS4 (在密码套件中仍使用此名称)
//   - 分组长度: 128 位 (16 字节)
//   - 密钥长度: 128 位 (16 字节)
//   - 对应国际标准: AES-128
//   - 应用场景:
//     * TLS 记录层对称加密
//     * 数据加密存储
//     * 虚拟专用网络 (VPN)
//
// 【工作模式】
//   - CBC 模式: 密码分组链接模式 (本文套件)
//   - GCM 模式: Galois/Counter 模式 (认证加密，带 AEAD)
//
// 【在密码套件中的位置】
//   套件 "SM2-WITH-SMS4-GCM-SM3" 中:
//     - SM2:    密钥交换和认证
//     - SMS4:   对称加密 (即 SM4，GCM 模式)
//     - SM3:    哈希算法和 PRF
//
TEST_F(IntegrationTest, IT024_TlsGMSSL_Sm4Cipher) {
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
            "sm2_cert_path": "",
            "sm2_key_path": "",
            "sm2_sign_cert_path": "",
            "sm2_sign_key_path": "",
            "cipher_suite": "SM2-WITH-SMS4-GCM-SM3"
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

    // 使用 SM4 (SMS4) 对称加密算法的密码套件: SM2-WITH-SMS4-GCM-SM3
    // GCM 模式提供认证加密 (AEAD)，同时保证机密性和完整性
    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "POST", "/api/gmssl_sm4",
        {{"Content-Type", "application/json"}},
        "{\"cipher\": \"SMS4\", \"mode\": \"GCM\", \"description\": \"国密分组密码算法，128位分组/密钥\"}"
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

// IT-025: TLS 国密 ECDHE-SM2-WITH-SMS4-SM3 套件
//
// 【密码套件解析】
//   ECDHE-SM2-WITH-SMS4-SM3
//   |      |    |    |    |
//   |      |    |    |    +-- SM3: 哈希算法 (用于 PRF 和签名)
//   |      |    |    +------- SMS4: 对称加密算法 (即 SM4)
//   |      |    +------------ WITH: 分隔符
//   |      +----------------- SM2: 认证算法 (签名)
//   +------------------------ ECDHE: 密钥交换算法 (带前向安全性)
//
// 【前向安全性 (Forward Secrecy)】
//   - ECDHE = Ephemeral Elliptic Curve Diffie-Hellman
//   - 特点: 每次会话生成临时密钥对，即使长期私钥泄露也不会影响历史会话
//   - 对比: 普通 SM2 密钥交换不具备前向安全性
//
// 【套件安全特性】
//   - 密钥交换: ECDHE-SM2 (前向安全)
//   - 认证: SM2 签名
//   - 加密: SMS4 (SM4) CBC 模式
//   - 哈希: SM3
//
TEST_F(IntegrationTest, IT025_TlsGMSSL_EcdheSm2Suite) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    TempFile config_file(get_valid_config_json(server_port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 使用带前向安全性的国密密码套件
    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "GET", "/api/gmssl_ecdhe_sm2",
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

// IT-026: TLS 国密 ECDHE-SM2-WITH-SMS4-GCM-SM3 套件
//
// 【密码套件解析】
//   ECDHE-SM2-WITH-SMS4-GCM-SM3
//   |      |    |    |    |   |
//   |      |    |    |    |   +-- SM3: 哈希算法
//   |      |    |    |    +------ GCM: 认证加密模式
//   |      |    |    +----------- SMS4: 对称加密算法 (即 SM4)
//   |      |    +---------------- WITH: 分隔符
//   |      +--------------------- SM2: 认证算法 (签名)
//   +---------------------------- ECDHE: 密钥交换算法 (带前向安全性)
//
// 【GCM 模式特点】
//   - Galois/Counter Mode (GCM)
//   - 类型: AEAD (Authenticated Encryption with Associated Data)
//   - 功能: 同时提供加密和认证
//   - 优势:
//     * 并行处理，高性能
//     * 内置消息认证码 (MAC)
//     * 防止密文篡改
//
// 【套件安全特性】
//   - 密钥交换: ECDHE-SM2 (前向安全)
//   - 认证: SM2 签名
//   - 加密: SMS4 (SM4) GCM 模式 (认证加密)
//   - 哈希: SM3
//   - 这是最强的国密密码套件配置
//
TEST_F(IntegrationTest, IT026_TlsGMSSL_EcdheSm2GcmSuite) {
    uint16_t server_port = get_test_port();
    uint16_t client_port = client_port_counter.fetch_add(1);
    TempFile config_file(get_valid_config_json(server_port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // 使用带 GCM 认证加密和前向安全性的国密密码套件
    SimClientRunner runner(test_dir_);
    std::string client_config = SimClientRunner::build_single_request_config(
        client_port, server_port, "POST", "/api/gmssl_ecdhe_sm2_gcm",
        {{"Content-Type", "application/json"}},
        "{\"suite\": \"ECDHE-SM2-WITH-SMS4-GCM-SM3\", \"mode\": \"GCM\", \"features\": [\"前向安全性\", \"认证加密\"]}"
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

// IT-027: TLS 国密 SM9 标识密码算法（预留）
//
// 【SM9 算法说明】
//   - 类型: 标识密码算法 (Identity-Based Cryptography, IBC)
//   - 设计者: 中国国家密码管理局
//   - 发布时间: 2016 年
//   - 对应国际标准: IEEE P1363.3 / ISO/IEC 14888-3
//
// 【核心思想】
//   - 使用用户标识（如邮箱、手机号、身份证号）直接作为公钥
//   - 无需 CA 颁发数字证书
//   - 由密钥生成中心 (KGC, Key Generation Center) 生成私钥
//
// 【SM9 组成部分】
//   - SM9-1: 基于标识的密钥封装机制 (IB-KEM)
//   - SM9-2: 基于标识的数字签名算法
//   - SM9-3: 基于标识的密钥交换协议
//
// 【应用场景】
//   - 安全电子邮件 (使用邮箱作为公钥)
//   - 即时通讯 (使用用户ID作为公钥)
//   - 物联网设备认证
//   - 云存储加密
//
// 【在 TLS 中的应用】
//   - 目前 TLS 标准中较少使用 SM9
//   - 主要用于特定的国密合规场景
//   - 常见国密 TLS 仍以 SM2+SM3+SM4 为主
//
// 【与 SM2 对比】
//   | 特性           | SM2                  | SM9                  |
//   |----------------|----------------------|----------------------|
//   | 公钥来源       | 证书                 | 用户标识             |
//   | 需要证书       | 是                   | 否                   |
//   | 需要 KGC       | 否                   | 是                   |
//   | TLS 应用       | 广泛                 | 较少                 |
//
TEST_F(IntegrationTest, IT027_TlsGMSSL_Sm9IdentityBased) {
    uint16_t server_port = get_test_port();
    (void)client_port_counter;
    TempFile config_file(get_valid_config_json(server_port));
    server::Server server;
    int ret = server.init(config_file.path());
    ASSERT_EQ(ret, 0);
    ret = server.start();
    ASSERT_EQ(ret, 0);

    // SM9 测试预留：本项目当前使用 SM2+SM3+SM4 国密套件
    // SM9 主要用于基于标识的密码系统，不依赖证书
    server::ServerStatus status;
    server.get_status(&status);
    EXPECT_EQ(status.status, server::SERVER_STATUS_RUNNING);

    ret = server.stop();
    EXPECT_EQ(ret, 0);
    server.cleanup();
}

// IT-051 ~ IT-060: 更多 TLS 测试占位
TEST_F(IntegrationTest, IT051_TlsRenegotiation) {
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

TEST_F(IntegrationTest, IT052_TlsCertChain_Complete) {
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

TEST_F(IntegrationTest, IT053_TlsCertChain_MissingIntermediate) {
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

TEST_F(IntegrationTest, IT054_TlsSelfSignedCert) {
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

TEST_F(IntegrationTest, IT055_TlsSessionResumption_SessionId) {
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

TEST_F(IntegrationTest, IT056_TlsSessionResumption_SessionTicket) {
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

TEST_F(IntegrationTest, IT057_TlsAlpn_Success) {
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

TEST_F(IntegrationTest, IT058_TlsAlpn_NoOverlap) {
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

TEST_F(IntegrationTest, IT059_TlsSni) {
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

TEST_F(IntegrationTest, IT060_ConnectionIdleTimeout) {
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
