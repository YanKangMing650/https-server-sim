#include "debug_chain/debug_chain.hpp"
#include <random>
#include <thread>
#include <algorithm>

namespace https_server_sim {
namespace debug_chain {

// DebugPointConfig
DebugPointConfig::DebugPointConfig()
    : server_port(0)
    , action(DebugActionType::NONE)
    , delay_ms(0)
    , error_code(0)
    , probability(100)
    , enabled(false)
{
}

// DebugChainConfig
DebugChainConfig::DebugChainConfig()
    : enabled_(false)
{
}

DebugChainConfig::~DebugChainConfig() {
}

void DebugChainConfig::set_enabled(bool enabled) {
    enabled_ = enabled;
}

bool DebugChainConfig::is_enabled() const {
    return enabled_;
}

void DebugChainConfig::add_point(const DebugPointConfig& point) {
    points_.push_back(point);
}

void DebugChainConfig::remove_point(const std::string& name) {
    auto it = std::remove_if(points_.begin(), points_.end(),
        [&name](const DebugPointConfig& p) { return p.name == name; });
    points_.erase(it, points_.end());
}

void DebugChainConfig::remove_point(uint16_t server_port, const std::string& name) {
    auto it = std::remove_if(points_.begin(), points_.end(),
        [server_port, &name](const DebugPointConfig& p) {
            return p.server_port == server_port && p.name == name;
        });
    points_.erase(it, points_.end());
}

const DebugPointConfig* DebugChainConfig::get_point(const std::string& name) const {
    for (const auto& p : points_) {
        if (p.name == name) {
            return &p;
        }
    }
    return nullptr;
}

const DebugPointConfig* DebugChainConfig::get_point(uint16_t server_port, const std::string& name) const {
    for (const auto& p : points_) {
        if (p.server_port == server_port && p.name == name) {
            return &p;
        }
    }
    for (const auto& p : points_) {
        if (p.server_port == 0 && p.name == name) {
            return &p;
        }
    }
    return nullptr;
}

std::vector<const DebugPointConfig*> DebugChainConfig::get_points_for_port(uint16_t server_port) const {
    std::vector<const DebugPointConfig*> result;
    for (const auto& p : points_) {
        if (p.server_port == server_port || p.server_port == 0) {
            result.push_back(&p);
        }
    }
    return result;
}

const std::vector<DebugPointConfig>& DebugChainConfig::get_all_points() const {
    return points_;
}

void DebugChainConfig::clear() {
    points_.clear();
}

void DebugChainConfig::reset() {
    enabled_ = false;
    points_.clear();
}

// DebugContext
DebugContext::DebugContext()
    : connection_id_(0)
    , server_port_(0)
    , skipped_(false)
    , error_code_(0)
    , delay_ms_(0)
{
}

DebugContext::~DebugContext() {
}

uint64_t DebugContext::get_connection_id() const { return connection_id_; }
void DebugContext::set_connection_id(uint64_t id) { connection_id_ = id; }

uint16_t DebugContext::get_server_port() const { return server_port_; }
void DebugContext::set_server_port(uint16_t port) { server_port_ = port; }

const std::string& DebugContext::get_point_name() const { return point_name_; }
void DebugContext::set_point_name(const std::string& name) { point_name_ = name; }

bool DebugContext::is_skipped() const { return skipped_; }
void DebugContext::set_skipped(bool skipped) { skipped_ = skipped; }

int32_t DebugContext::get_error_code() const { return error_code_; }
void DebugContext::set_error_code(int32_t code) { error_code_ = code; }

uint32_t DebugContext::get_delay_ms() const { return delay_ms_; }
void DebugContext::set_delay_ms(uint32_t ms) { delay_ms_ = ms; }

void DebugContext::add_log(const std::string& message) { logs_.push_back(message); }
const std::vector<std::string>& DebugContext::get_logs() const { return logs_; }

void DebugContext::reset() {
    connection_id_ = 0;
    server_port_ = 0;
    point_name_.clear();
    skipped_ = false;
    error_code_ = 0;
    delay_ms_ = 0;
    logs_.clear();
}

// DebugHandler
void DebugHandler::set_next(std::shared_ptr<DebugHandler> next) {
    next_ = next;
}

std::shared_ptr<DebugHandler> DebugHandler::get_next() const {
    return next_;
}

// DelayHandler
DelayHandler::DelayHandler() {}
DelayHandler::~DelayHandler() {}

const std::string& DelayHandler::get_name() const {
    static const std::string name = "delay";
    return name;
}

bool DelayHandler::handle(DebugContext& context) {
    if (context.get_delay_ms() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(context.get_delay_ms()));
    }
    if (next_) {
        return next_->handle(context);
    }
    return true;
}

// DisconnectHandler
DisconnectHandler::DisconnectHandler() {}
DisconnectHandler::~DisconnectHandler() {}

const std::string& DisconnectHandler::get_name() const {
    static const std::string name = "disconnect";
    return name;
}

bool DisconnectHandler::handle(DebugContext& context) {
    context.set_skipped(true);
    return false;
}

// ErrorCodeHandler
ErrorCodeHandler::ErrorCodeHandler() {}
ErrorCodeHandler::~ErrorCodeHandler() {}

const std::string& ErrorCodeHandler::get_name() const {
    static const std::string name = "error_code";
    return name;
}

bool ErrorCodeHandler::handle(DebugContext& context) {
    if (next_) {
        return next_->handle(context);
    }
    return true;
}

// LogHandler
LogHandler::LogHandler() {}
LogHandler::~LogHandler() {}

const std::string& LogHandler::get_name() const {
    static const std::string name = "log";
    return name;
}

bool LogHandler::handle(DebugContext& context) {
    context.add_log("Debug point: " + context.get_point_name());
    if (next_) {
        return next_->handle(context);
    }
    return true;
}

// DebugChain
DebugChain::DebugChain()
    : enabled_(false)
{
    build_chain();
}

DebugChain::~DebugChain() {}

void DebugChain::set_config(const DebugChainConfig& config) {
    config_ = config;
    enabled_ = config.is_enabled();
}

const DebugChainConfig& DebugChain::get_config() const {
    return config_;
}

void DebugChain::set_enabled(bool enabled) {
    enabled_ = enabled;
}

bool DebugChain::is_enabled() const {
    return enabled_;
}

bool DebugChain::execute(uint16_t server_port, const std::string& point_name, DebugContext& context) {
    if (!enabled_) {
        return true;
    }

    const DebugPointConfig* point_config = config_.get_point(server_port, point_name);
    if (!point_config || !point_config->enabled) {
        return true;
    }

    if (!should_trigger(point_config->probability)) {
        return true;
    }

    context.set_point_name(point_name);
    context.set_server_port(server_port);

    switch (point_config->action) {
        case DebugActionType::DELAY:
            context.set_delay_ms(point_config->delay_ms);
            break;
        case DebugActionType::RETURN_ERROR:
            context.set_error_code(point_config->error_code);
            break;
        default:
            break;
    }

    if (chain_head_) {
        return chain_head_->handle(context);
    }
    return true;
}

bool DebugChain::execute_simple(uint64_t connection_id, uint16_t server_port, const std::string& point_name) {
    DebugContext context;
    context.set_connection_id(connection_id);
    return execute(server_port, point_name, context);
}

void DebugChain::build_chain() {
    auto log = std::make_shared<LogHandler>();
    auto delay = std::make_shared<DelayHandler>();
    auto error = std::make_shared<ErrorCodeHandler>();
    auto disconnect = std::make_shared<DisconnectHandler>();

    log->set_next(delay);
    delay->set_next(error);
    error->set_next(disconnect);

    chain_head_ = log;
}

bool DebugChain::should_trigger(uint32_t probability) const {
    if (probability >= 100) {
        return true;
    }
    if (probability == 0) {
        return false;
    }

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, 100);
    return dist(gen) <= probability;
}

// DebugChainManager
DebugChainManager& DebugChainManager::instance() {
    static DebugChainManager mgr;
    return mgr;
}

DebugChainManager::DebugChainManager() {}
DebugChainManager::~DebugChainManager() {}

DebugChain& DebugChainManager::get_chain() {
    return chain_;
}

bool DebugChainManager::execute(uint16_t server_port, const std::string& point_name, DebugContext& context) {
    return chain_.execute(server_port, point_name, context);
}

bool DebugChainManager::execute_simple(uint64_t connection_id, uint16_t server_port, const std::string& point_name) {
    return chain_.execute_simple(connection_id, server_port, point_name);
}

} // namespace debug_chain
} // namespace https_server_sim
