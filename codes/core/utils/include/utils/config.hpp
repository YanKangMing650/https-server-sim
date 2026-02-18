#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "utils/error.hpp"

namespace https_server_sim {
namespace utils {

// ========== 配置数据结构 ==========

struct ServerConfig {
    std::string listen_ip = "0.0.0.0";
    uint16_t listen_port = 8443;
    std::string tls_version = "both";
    uint32_t thread_count = 5;
    uint32_t timeout_seconds = 30;
};

struct CertificatesConfig {
    std::string cert_file;
    std::string key_file;
    std::string cert_type = "international";
    std::vector<std::string> cipher_suites;
};

struct DebugConfig {
    bool enabled = false;
    bool log_packets = false;
};

struct CallbacksConfig {
    std::string strategy = "port_based";
    std::unordered_map<std::string, std::string> port_mapping;
};

struct LoggingConfig {
    std::string level = "INFO";
    std::string file;
    uint32_t max_size_mb = 100;
    uint32_t max_files = 10;
};

struct Http2Config {
    bool enabled = true;
    uint32_t max_concurrent_streams = 100;
    uint32_t initial_window_size = 65535;
};

// ========== Config主类（防腐层） ==========
// 注意：头文件不包含nlohmann/json.hpp，完全隔离外部依赖

class Config {
public:
    Config();
    ~Config();

    // 禁止拷贝
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    // 支持移动
    Config(Config&& other) noexcept;
    Config& operator=(Config&& other) noexcept;

    // ========== 从JSON文件/字符串加载（防腐层接口） ==========
    // 注：实际JSON解析在config.cpp中实现，头文件不暴露nlohmann/json

    // 从文件加载配置
    // file_path: JSON配置文件路径
    // return: 成功返回SUCCESS，失败返回错误码
    Result<void> load_from_file(const std::string& file_path);

    // 从JSON字符串加载配置
    // json_str: JSON格式字符串
    // return: 成功返回SUCCESS，失败返回错误码
    Result<void> load_from_string(const std::string& json_str);

    // 验证配置合法性
    // return: 成功返回SUCCESS，失败返回错误码
    Result<void> validate() const;

    // ========== 获取配置项 ==========

    const ServerConfig& get_server() const { return server_; }
    const CertificatesConfig& get_certificates() const { return certificates_; }
    const DebugConfig& get_debug() const { return debug_; }
    const CallbacksConfig& get_callbacks() const { return callbacks_; }
    const LoggingConfig& get_logging() const { return logging_; }
    const Http2Config& get_http2() const { return http2_; }

    // ========== 设置配置项（供Server模块使用） ==========

    void set_server(const ServerConfig& cfg) { server_ = cfg; }
    void set_certificates(const CertificatesConfig& cfg) { certificates_ = cfg; }
    void set_debug(const DebugConfig& cfg) { debug_ = cfg; }
    void set_callbacks(const CallbacksConfig& cfg) { callbacks_ = cfg; }
    void set_logging(const LoggingConfig& cfg) { logging_ = cfg; }
    void set_http2(const Http2Config& cfg) { http2_ = cfg; }

    // ========== 导出配置 ==========

    // 导出为JSON字符串
    // return: 成功返回JSON字符串，失败返回错误码
    Result<std::string> to_json_string() const;

private:
    ServerConfig server_;
    CertificatesConfig certificates_;
    DebugConfig debug_;
    CallbacksConfig callbacks_;
    LoggingConfig logging_;
    Http2Config http2_;
};

} // namespace utils
} // namespace https_server_sim
