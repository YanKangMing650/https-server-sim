#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace https_server_sim {
namespace config {

// 监听配置
struct ListenConfig {
    std::string ip;
    uint16_t port;
    bool enabled;
    uint32_t backlog;

    ListenConfig();
};

// 证书配置
struct CertificatesConfig {
    std::string cert_path;
    std::string key_path;
    std::string ca_path;
    std::string sm2_cert_path;
    std::string sm2_key_path;
    std::string sm2_sign_cert_path;
    std::string sm2_sign_key_path;
    std::string cipher_suite;

    CertificatesConfig();
};

// 调试配置
struct DebugPointConfig {
    uint16_t server_port;
    std::string point_name;
    std::string action;
    uint32_t delay_ms;
    int32_t error_code;
    uint32_t probability;

    DebugPointConfig();
};

struct DebugConfig {
    bool enabled;
    std::vector<DebugPointConfig> debug_points;

    DebugConfig();
};

// 回调配置
struct CallbackConfig {
    uint16_t server_port;
    std::string script_path;

    CallbackConfig();
};

struct CallbacksConfig {
    std::string callbacks_dir;
    std::vector<CallbackConfig> callbacks;

    CallbacksConfig();
};

// 日志配置
struct LoggingConfig {
    std::string level;
    std::string file;
    bool console_output;

    LoggingConfig();
};

// HTTP/2配置
struct Http2Config {
    bool enabled;
    bool allow_h2c;

    Http2Config();
};

// 主配置类
class Config {
public:
    Config();
    ~Config();

    // 从JSON文件加载配置
    int load_from_file(const std::string& config_path);

    // 从JSON字符串加载配置
    int load_from_string(const std::string& json_str);

    // 验证配置
    int validate() const;

    // 获取配置项
    const std::vector<ListenConfig>& get_listens() const;
    const CertificatesConfig& get_certificates() const;
    const DebugConfig& get_debug() const;
    const CallbacksConfig& get_callbacks() const;
    const LoggingConfig& get_logging() const;
    const Http2Config& get_http2() const;

    // 设置配置项
    void set_listens(const std::vector<ListenConfig>& listens);
    void set_certificates(const CertificatesConfig& certificates);
    void set_debug(const DebugConfig& debug);
    void set_callbacks(const CallbacksConfig& callbacks);
    void set_logging(const LoggingConfig& logging);
    void set_http2(const Http2Config& http2);

    // 获取/设置回调目录
    const std::string& get_callbacks_dir() const;
    void set_callbacks_dir(const std::string& dir);

    // 重置为默认配置
    void reset();

private:
    std::vector<ListenConfig> listens_;
    CertificatesConfig certificates_;
    DebugConfig debug_;
    CallbacksConfig callbacks_;
    LoggingConfig logging_;
    Http2Config http2_;
    std::string callbacks_dir_;
};

} // namespace config
} // namespace https_server_sim
