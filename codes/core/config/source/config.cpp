#include "config/config.hpp"
#include <fstream>
#include <sstream>
#include "nlohmann/json.hpp"

namespace https_server_sim {
namespace config {

using Json = nlohmann::json;

namespace details {

// 解析ListenConfig
void ParseListenConfig(const Json& j, ListenConfig& cfg) {
    if (j.contains("ip") && j["ip"].is_string()) {
        cfg.ip = j["ip"].get<std::string>();
    }
    if (j.contains("port") && j["port"].is_number()) {
        cfg.port = j["port"].get<uint16_t>();
    }
    if (j.contains("enabled") && j["enabled"].is_boolean()) {
        cfg.enabled = j["enabled"].get<bool>();
    }
    if (j.contains("backlog") && j["backlog"].is_number()) {
        cfg.backlog = j["backlog"].get<uint32_t>();
    }
}

// 解析CertificatesConfig
void ParseCertificatesConfig(const Json& j, CertificatesConfig& cfg) {
    if (j.contains("cert_path") && j["cert_path"].is_string()) {
        cfg.cert_path = j["cert_path"].get<std::string>();
    }
    if (j.contains("key_path") && j["key_path"].is_string()) {
        cfg.key_path = j["key_path"].get<std::string>();
    }
    if (j.contains("ca_path") && j["ca_path"].is_string()) {
        cfg.ca_path = j["ca_path"].get<std::string>();
    }
    if (j.contains("sm2_cert_path") && j["sm2_cert_path"].is_string()) {
        cfg.sm2_cert_path = j["sm2_cert_path"].get<std::string>();
    }
    if (j.contains("sm2_key_path") && j["sm2_key_path"].is_string()) {
        cfg.sm2_key_path = j["sm2_key_path"].get<std::string>();
    }
    if (j.contains("sm2_sign_cert_path") && j["sm2_sign_cert_path"].is_string()) {
        cfg.sm2_sign_cert_path = j["sm2_sign_cert_path"].get<std::string>();
    }
    if (j.contains("sm2_sign_key_path") && j["sm2_sign_key_path"].is_string()) {
        cfg.sm2_sign_key_path = j["sm2_sign_key_path"].get<std::string>();
    }
    if (j.contains("cipher_suite") && j["cipher_suite"].is_string()) {
        cfg.cipher_suite = j["cipher_suite"].get<std::string>();
    }
}

// 解析DebugPointConfig
void ParseDebugPointConfig(const Json& j, DebugPointConfig& cfg) {
    if (j.contains("server_port") && j["server_port"].is_number()) {
        cfg.server_port = j["server_port"].get<uint16_t>();
    }
    if (j.contains("point_name") && j["point_name"].is_string()) {
        cfg.point_name = j["point_name"].get<std::string>();
    }
    if (j.contains("action") && j["action"].is_string()) {
        cfg.action = j["action"].get<std::string>();
    }
    if (j.contains("delay_ms") && j["delay_ms"].is_number()) {
        cfg.delay_ms = j["delay_ms"].get<uint32_t>();
    }
    if (j.contains("error_code") && j["error_code"].is_number()) {
        cfg.error_code = j["error_code"].get<int32_t>();
    }
    if (j.contains("probability") && j["probability"].is_number()) {
        cfg.probability = j["probability"].get<uint32_t>();
    }
}

// 解析DebugConfig
void ParseDebugConfig(const Json& j, DebugConfig& cfg) {
    if (j.contains("enabled") && j["enabled"].is_boolean()) {
        cfg.enabled = j["enabled"].get<bool>();
    }
    if (j.contains("debug_points") && j["debug_points"].is_array()) {
        cfg.debug_points.clear();
        for (const auto& dp : j["debug_points"]) {
            DebugPointConfig point;
            ParseDebugPointConfig(dp, point);
            cfg.debug_points.push_back(point);
        }
    }
}

// 解析CallbackConfig
void ParseCallbackConfig(const Json& j, CallbackConfig& cfg) {
    if (j.contains("server_port") && j["server_port"].is_number()) {
        cfg.server_port = j["server_port"].get<uint16_t>();
    }
    if (j.contains("script_path") && j["script_path"].is_string()) {
        cfg.script_path = j["script_path"].get<std::string>();
    }
}

// 解析CallbacksConfig
void ParseCallbacksConfig(const Json& j, CallbacksConfig& cfg) {
    if (j.contains("callbacks_dir") && j["callbacks_dir"].is_string()) {
        cfg.callbacks_dir = j["callbacks_dir"].get<std::string>();
    }
    if (j.contains("callbacks") && j["callbacks"].is_array()) {
        cfg.callbacks.clear();
        for (const auto& cb : j["callbacks"]) {
            CallbackConfig callback;
            ParseCallbackConfig(cb, callback);
            cfg.callbacks.push_back(callback);
        }
    }
}

// 解析LoggingConfig
void ParseLoggingConfig(const Json& j, LoggingConfig& cfg) {
    if (j.contains("level") && j["level"].is_string()) {
        cfg.level = j["level"].get<std::string>();
    }
    if (j.contains("file") && j["file"].is_string()) {
        cfg.file = j["file"].get<std::string>();
    }
    if (j.contains("console_output") && j["console_output"].is_boolean()) {
        cfg.console_output = j["console_output"].get<bool>();
    }
}

// 解析Http2Config
void ParseHttp2Config(const Json& j, Http2Config& cfg) {
    if (j.contains("enabled") && j["enabled"].is_boolean()) {
        cfg.enabled = j["enabled"].get<bool>();
    }
    if (j.contains("allow_h2c") && j["allow_h2c"].is_boolean()) {
        cfg.allow_h2c = j["allow_h2c"].get<bool>();
    }
}

} // namespace details

// ListenConfig
ListenConfig::ListenConfig()
    : ip("0.0.0.0")
    , port(8443)
    , enabled(true)
    , backlog(128)
{
}

// CertificatesConfig
CertificatesConfig::CertificatesConfig()
    : cipher_suite("TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256")
{
}

// DebugPointConfig
DebugPointConfig::DebugPointConfig()
    : server_port(0)
    , delay_ms(0)
    , error_code(0)
    , probability(100)
{
}

// DebugConfig
DebugConfig::DebugConfig()
    : enabled(false)
{
}

// CallbackConfig
CallbackConfig::CallbackConfig()
    : server_port(0)
{
}

// CallbacksConfig
CallbacksConfig::CallbacksConfig()
    : callbacks_dir("callbacks")
{
}

// LoggingConfig
LoggingConfig::LoggingConfig()
    : level("INFO")
    , console_output(true)
{
}

// Http2Config
Http2Config::Http2Config()
    : enabled(true)
    , allow_h2c(false)
{
}

// Config
Config::Config() {
    reset();
}

Config::~Config() {
}

int Config::load_from_file(const std::string& config_path) {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        return -1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return load_from_string(buffer.str());
}

int Config::load_from_string(const std::string& json_str) {
    try {
        Json j = Json::parse(json_str);

        // 解析listens
        if (j.contains("listens") && j["listens"].is_array()) {
            listens_.clear();
            for (const auto& item : j["listens"]) {
                ListenConfig listen;
                details::ParseListenConfig(item, listen);
                listens_.push_back(listen);
            }
        }

        // 解析certificates
        if (j.contains("certificates") && j["certificates"].is_object()) {
            details::ParseCertificatesConfig(j["certificates"], certificates_);
        }

        // 解析debug
        if (j.contains("debug") && j["debug"].is_object()) {
            details::ParseDebugConfig(j["debug"], debug_);
        }

        // 解析callbacks
        if (j.contains("callbacks") && j["callbacks"].is_object()) {
            details::ParseCallbacksConfig(j["callbacks"], callbacks_);
            callbacks_dir_ = callbacks_.callbacks_dir;
        }

        // 解析logging
        if (j.contains("logging") && j["logging"].is_object()) {
            details::ParseLoggingConfig(j["logging"], logging_);
        }

        // 解析http2
        if (j.contains("http2") && j["http2"].is_object()) {
            details::ParseHttp2Config(j["http2"], http2_);
        }

        return 0;
    } catch (const Json::parse_error& e) {
        return -1;
    } catch (const Json::type_error& e) {
        return -1;
    } catch (...) {
        return -1;
    }
}

int Config::validate() const {
    if (listens_.empty()) {
        return -1;
    }
    for (const auto& listen : listens_) {
        if (listen.port == 0) {
            return -1;
        }
    }
    return 0;
}

const std::vector<ListenConfig>& Config::get_listens() const {
    return listens_;
}

const CertificatesConfig& Config::get_certificates() const {
    return certificates_;
}

const DebugConfig& Config::get_debug() const {
    return debug_;
}

const CallbacksConfig& Config::get_callbacks() const {
    return callbacks_;
}

const LoggingConfig& Config::get_logging() const {
    return logging_;
}

const Http2Config& Config::get_http2() const {
    return http2_;
}

void Config::set_listens(const std::vector<ListenConfig>& listens) {
    listens_ = listens;
}

void Config::set_certificates(const CertificatesConfig& certificates) {
    certificates_ = certificates;
}

void Config::set_debug(const DebugConfig& debug) {
    debug_ = debug;
}

void Config::set_callbacks(const CallbacksConfig& callbacks) {
    callbacks_ = callbacks;
}

void Config::set_logging(const LoggingConfig& logging) {
    logging_ = logging;
}

void Config::set_http2(const Http2Config& http2) {
    http2_ = http2;
}

const std::string& Config::get_callbacks_dir() const {
    return callbacks_dir_;
}

void Config::set_callbacks_dir(const std::string& dir) {
    callbacks_dir_ = dir;
}

void Config::reset() {
    listens_.clear();
    ListenConfig default_listen;
    listens_.push_back(default_listen);
    certificates_ = CertificatesConfig();
    debug_ = DebugConfig();
    callbacks_ = CallbacksConfig();
    logging_ = LoggingConfig();
    http2_ = Http2Config();
    callbacks_dir_ = "callbacks";
}

} // namespace config
} // namespace https_server_sim
