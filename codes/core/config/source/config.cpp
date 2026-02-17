#include "config/config.hpp"

namespace https_server_sim {
namespace config {

// ListenConfig
ListenConfig::ListenConfig()
    : ip("0.0.0.0")
    , port(8443)
    , enabled(true)
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

bool Config::load_from_file(const std::string& config_path) {
    (void)config_path;
    return true;
}

bool Config::load_from_string(const std::string& json_str) {
    (void)json_str;
    return true;
}

bool Config::validate() const {
    if (listens_.empty()) {
        return false;
    }
    for (const auto& listen : listens_) {
        if (listen.port == 0) {
            return false;
        }
    }
    return true;
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
