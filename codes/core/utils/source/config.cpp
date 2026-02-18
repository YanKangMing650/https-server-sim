#include "utils/config.hpp"
#include <fstream>
#include <sstream>

// 仅在cpp文件中包含nlohmann/json，头文件不暴露
#include <nlohmann/json.hpp>

namespace https_server_sim {
namespace utils {

using json = nlohmann::json;

namespace details {

// 辅助函数：解析json到配置结构体
Result<void> parse_json_to_config(const json& j,
                                 ServerConfig& server,
                                 CertificatesConfig& certificates,
                                 DebugConfig& debug,
                                 CallbacksConfig& callbacks,
                                 LoggingConfig& logging,
                                 Http2Config& http2) {
    try {
        // server (可选，有默认值)
        if (j.contains("server")) {
            const auto& s = j["server"];
            if (s.contains("listen_ip")) server.listen_ip = s["listen_ip"].get<std::string>();
            if (s.contains("listen_port")) server.listen_port = s["listen_port"].get<uint16_t>();
            if (s.contains("tls_version")) server.tls_version = s["tls_version"].get<std::string>();
            if (s.contains("thread_count")) server.thread_count = s["thread_count"].get<uint32_t>();
            if (s.contains("timeout_seconds")) server.timeout_seconds = s["timeout_seconds"].get<uint32_t>();
        }

        // certificates
        if (j.contains("certificates")) {
            const auto& c = j["certificates"];
            if (c.contains("cert_file")) certificates.cert_file = c["cert_file"].get<std::string>();
            if (c.contains("key_file")) certificates.key_file = c["key_file"].get<std::string>();
            if (c.contains("cert_type")) certificates.cert_type = c["cert_type"].get<std::string>();
            if (c.contains("cipher_suites")) {
                certificates.cipher_suites = c["cipher_suites"].get<std::vector<std::string>>();
            }
        }

        // debug
        if (j.contains("debug")) {
            const auto& d = j["debug"];
            if (d.contains("enabled")) debug.enabled = d["enabled"].get<bool>();
            if (d.contains("log_packets")) debug.log_packets = d["log_packets"].get<bool>();
        }

        // callbacks
        if (j.contains("callbacks")) {
            const auto& cb = j["callbacks"];
            if (cb.contains("strategy")) callbacks.strategy = cb["strategy"].get<std::string>();
            if (cb.contains("port_mapping")) {
                callbacks.port_mapping = cb["port_mapping"].get<std::unordered_map<std::string, std::string>>();
            }
        }

        // logging
        if (j.contains("logging")) {
            const auto& l = j["logging"];
            if (l.contains("level")) logging.level = l["level"].get<std::string>();
            if (l.contains("file")) logging.file = l["file"].get<std::string>();
            if (l.contains("max_size_mb")) logging.max_size_mb = l["max_size_mb"].get<uint32_t>();
            if (l.contains("max_files")) logging.max_files = l["max_files"].get<uint32_t>();
        }

        // http2
        if (j.contains("http2")) {
            const auto& h = j["http2"];
            if (h.contains("enabled")) http2.enabled = h["enabled"].get<bool>();
            if (h.contains("max_concurrent_streams")) {
                http2.max_concurrent_streams = h["max_concurrent_streams"].get<uint32_t>();
            }
            if (h.contains("initial_window_size")) {
                http2.initial_window_size = h["initial_window_size"].get<uint32_t>();
            }
        }

        return make_ok();
    } catch (const json::type_error& e) {
        return make_err(ErrorCode::CONFIG_INVALID_VALUE, std::string("JSON type error: ") + e.what());
    } catch (const std::exception& e) {
        return make_err(ErrorCode::CONFIG_PARSE_ERROR, std::string("Failed to parse config: ") + e.what());
    } catch (...) {
        return make_err(ErrorCode::CONFIG_PARSE_ERROR, "Failed to parse config: unknown error");
    }
}

} // namespace details

Config::Config() = default;
Config::~Config() = default;

Config::Config(Config&& other) noexcept
    : server_(std::move(other.server_))
    , certificates_(std::move(other.certificates_))
    , debug_(std::move(other.debug_))
    , callbacks_(std::move(other.callbacks_))
    , logging_(std::move(other.logging_))
    , http2_(std::move(other.http2_))
{
}

Config& Config::operator=(Config&& other) noexcept {
    if (this != &other) {
        server_ = std::move(other.server_);
        certificates_ = std::move(other.certificates_);
        debug_ = std::move(other.debug_);
        callbacks_ = std::move(other.callbacks_);
        logging_ = std::move(other.logging_);
        http2_ = std::move(other.http2_);
    }
    return *this;
}

Result<void> Config::load_from_file(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return make_err(ErrorCode::FILE_NOT_FOUND, "Cannot open config file: " + file_path);
    }

    try {
        json j;
        file >> j;
        return details::parse_json_to_config(j, server_, certificates_, debug_, callbacks_, logging_, http2_);
    } catch (const json::parse_error& e) {
        return make_err(ErrorCode::CONFIG_PARSE_ERROR, std::string("JSON parse error: ") + e.what());
    } catch (const json::type_error& e) {
        return make_err(ErrorCode::CONFIG_INVALID_VALUE, std::string("JSON type error: ") + e.what());
    } catch (const std::exception& e) {
        return make_err(ErrorCode::OPERATION_FAILED, std::string("Failed to load config: ") + e.what());
    } catch (...) {
        return make_err(ErrorCode::OPERATION_FAILED, "Failed to load config: unknown error");
    }
}

Result<void> Config::load_from_string(const std::string& json_str) {
    try {
        json j = json::parse(json_str);
        return details::parse_json_to_config(j, server_, certificates_, debug_, callbacks_, logging_, http2_);
    } catch (const json::parse_error& e) {
        return make_err(ErrorCode::CONFIG_PARSE_ERROR, std::string("JSON parse error: ") + e.what());
    } catch (const json::type_error& e) {
        return make_err(ErrorCode::CONFIG_INVALID_VALUE, std::string("JSON type error: ") + e.what());
    } catch (const std::exception& e) {
        return make_err(ErrorCode::OPERATION_FAILED, std::string("Failed to load config: ") + e.what());
    } catch (...) {
        return make_err(ErrorCode::OPERATION_FAILED, "Failed to load config: unknown error");
    }
}

Result<void> Config::validate() const {
    // 验证端口
    if (server_.listen_port == 0 || server_.listen_port > 65535) {
        return make_err(ErrorCode::CONFIG_INVALID_PORT, "Invalid port: " + std::to_string(server_.listen_port));
    }

    // 验证线程数
    if (server_.thread_count == 0 || server_.thread_count > 1024) {
        return make_err(ErrorCode::CONFIG_INVALID_THREAD_COUNT, "Invalid thread count: " + std::to_string(server_.thread_count));
    }

    // 验证日志级别
    if (logging_.level != "DEBUG" && logging_.level != "INFO" &&
        logging_.level != "WARN" && logging_.level != "ERROR") {
        return make_err(ErrorCode::CONFIG_INVALID_LOG_LEVEL, "Invalid log level: " + logging_.level);
    }

    return make_ok();
}

Result<std::string> Config::to_json_string() const {
    try {
        json j;
        // server
        j["server"]["listen_ip"] = server_.listen_ip;
        j["server"]["listen_port"] = server_.listen_port;
        j["server"]["tls_version"] = server_.tls_version;
        j["server"]["thread_count"] = server_.thread_count;
        j["server"]["timeout_seconds"] = server_.timeout_seconds;

        // certificates
        j["certificates"]["cert_file"] = certificates_.cert_file;
        j["certificates"]["key_file"] = certificates_.key_file;
        j["certificates"]["cert_type"] = certificates_.cert_type;
        j["certificates"]["cipher_suites"] = certificates_.cipher_suites;

        // debug
        j["debug"]["enabled"] = debug_.enabled;
        j["debug"]["log_packets"] = debug_.log_packets;

        // callbacks
        j["callbacks"]["strategy"] = callbacks_.strategy;
        j["callbacks"]["port_mapping"] = callbacks_.port_mapping;

        // logging
        j["logging"]["level"] = logging_.level;
        j["logging"]["file"] = logging_.file;
        j["logging"]["max_size_mb"] = logging_.max_size_mb;
        j["logging"]["max_files"] = logging_.max_files;

        // http2
        j["http2"]["enabled"] = http2_.enabled;
        j["http2"]["max_concurrent_streams"] = http2_.max_concurrent_streams;
        j["http2"]["initial_window_size"] = http2_.initial_window_size;

        return make_ok(j.dump(4));
    } catch (...) {
        return make_err<std::string>(ErrorCode::OPERATION_FAILED, "Failed to serialize config to JSON");
    }
}

} // namespace utils
} // namespace https_server_sim
