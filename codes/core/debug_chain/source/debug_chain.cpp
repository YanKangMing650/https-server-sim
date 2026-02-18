#include "debug_chain/debug_chain.hpp"
#include "debug_chain/debug_handler.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <chrono>

namespace https_server_sim {
namespace debug_chain {

// ========== 常量定义 ==========
namespace {

// Handler优先级常量
constexpr int kDelayHandlerPriority = 100;
constexpr int kDisconnectHandlerPriority = 200;
constexpr int kLogHandlerPriority = 300;
constexpr int kErrorCodeHandlerPriority = 400;

// Handler名称常量
const char* const kDelayHandlerName = "DelayHandler";
const char* const kDisconnectHandlerName = "DisconnectHandler";
const char* const kLogHandlerName = "LogHandler";
const char* const kErrorCodeHandlerName = "ErrorCodeHandler";

// 返回值常量
constexpr int kRetSuccess = 0;
constexpr int kRetInvalidParam = -1;
constexpr int kRetStopChain = 1;

// 日志模块名称
const char* const kLoggerName = "DebugChain";

} // anonymous namespace

// ========== DebugConfig实现 ==========

DebugConfig::DebugConfig()
    : enabled(false)
    , delay_ms(0)
    , force_disconnect(false)
    , log_packet(false)
    , http_status(200)
{
}

// ========== DebugContext实现 ==========

DebugContext::DebugContext()
    : override_http_status(0)
    , skip_callback(false)
    , disconnect_after(false)
{
}

// ========== DebugChain实现 ==========

DebugChain::DebugChain()
    : sorted_(false)
{
}

DebugChain::~DebugChain()
{
    // 遍历handlers_，对非空destroy指针的handler调用destroy()
    for (auto handler : handlers_) {
        if (handler != nullptr && handler->destroy != nullptr) {
            handler->destroy(handler);
        }
    }
    handlers_.clear();
}

int DebugChain::register_handler(DebugHandler* handler)
{
    if (handler == nullptr) {
        return kRetInvalidParam;
    }
    // 检查handler->name非空
    if (handler->name == nullptr) {
        return kRetInvalidParam;
    }
    // 检查是否已存在同名handler
    if (has_handler(handler->name)) {
        return kRetInvalidParam;
    }
    handlers_.push_back(handler);
    sorted_ = false;
    return kRetSuccess;
}

bool DebugChain::has_handler(const char* name) const
{
    if (name == nullptr) {
        return false;
    }

    auto it = std::find_if(handlers_.begin(), handlers_.end(),
        [name](DebugHandler* handler) {
            return handler != nullptr && handler->name != nullptr &&
                   std::strcmp(handler->name, name) == 0;
        });

    return it != handlers_.end();
}

int DebugChain::unregister_handler(const char* name)
{
    if (name == nullptr) {
        return kRetInvalidParam;
    }

    auto it = std::find_if(handlers_.begin(), handlers_.end(),
        [name](DebugHandler* handler) {
            return handler != nullptr && handler->name != nullptr &&
                   std::strcmp(handler->name, name) == 0;
        });

    if (it == handlers_.end()) {
        return kRetInvalidParam;
    }

    // 调用destroy释放handler内存（所有create_*_handler都设置了destroy）
    DebugHandler* handler = *it;
    if (handler->destroy != nullptr) {
        handler->destroy(handler);
    }

    handlers_.erase(it);
    return kRetSuccess;
}

int DebugChain::process_request(const ClientContext* ctx,
                                const DebugConfig* config,
                                DebugContext* debug_ctx)
{
    // 参数校验
    if (ctx == nullptr || config == nullptr || debug_ctx == nullptr) {
        return kRetInvalidParam;
    }

    // 检查总开关
    if (!config->enabled) {
        return kRetSuccess;
    }

    // 排序（如需要）
    if (!sorted_) {
        sort_handlers();
    }

    // 遍历执行handler
    for (auto handler : handlers_) {
        if (handler == nullptr || handler->handle_request == nullptr) {
            continue;
        }

        int result = handler->handle_request(handler, ctx, config, debug_ctx);
        if (result != 0) {
            // 非0值：终止链
            return result;
        }
    }

    return kRetSuccess;
}

int DebugChain::process_response(const ClientContext* ctx,
                                 const DebugConfig* config,
                                 DebugContext* debug_ctx)
{
    // 参数校验
    if (ctx == nullptr || config == nullptr || debug_ctx == nullptr) {
        return kRetInvalidParam;
    }

    // 检查总开关
    if (!config->enabled) {
        return kRetSuccess;
    }

    // 排序（如需要）
    if (!sorted_) {
        sort_handlers();
    }

    // 遍历执行handler
    for (auto handler : handlers_) {
        if (handler == nullptr || handler->handle_response == nullptr) {
            continue;
        }

        int result = handler->handle_response(handler, ctx, config, debug_ctx);
        if (result != 0) {
            // 非0值：终止链
            return result;
        }
    }

    return kRetSuccess;
}

void DebugChain::sort_handlers()
{
    std::sort(handlers_.begin(), handlers_.end(),
        [](DebugHandler* a, DebugHandler* b) {
            // 按priority升序排序
            return a->priority < b->priority;
        });
    sorted_ = true;
}

// ========== DelayHandler实现 ==========

namespace details {

static int delay_handler_handle_request(DebugHandler* self,
                                        const ClientContext* ctx,
                                        const DebugConfig* config,
                                        DebugContext* debug_ctx)
{
    (void)self;
    (void)ctx;
    (void)debug_ctx;

    if (config == nullptr) {
        return kRetInvalidParam;
    }

    if (config->delay_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(config->delay_ms));
    }
    return kRetSuccess;
}

static int delay_handler_handle_response(DebugHandler* self,
                                         const ClientContext* ctx,
                                         const DebugConfig* config,
                                         DebugContext* debug_ctx)
{
    (void)self;
    (void)ctx;
    (void)debug_ctx;

    if (config == nullptr) {
        return kRetInvalidParam;
    }

    if (config->delay_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(config->delay_ms));
    }
    return kRetSuccess;
}

static void delay_handler_destroy(DebugHandler* self)
{
    if (self != nullptr) {
        std::free(self);
    }
}

} // namespace details

DebugHandler* create_delay_handler()
{
    DebugHandler* handler = (DebugHandler*)std::malloc(sizeof(DebugHandler));
    if (handler == nullptr) {
        return nullptr;
    }

    handler->name = kDelayHandlerName;
    handler->priority = kDelayHandlerPriority;
    handler->user_data = nullptr;
    handler->handle_request = details::delay_handler_handle_request;
    handler->handle_response = details::delay_handler_handle_response;
    handler->destroy = details::delay_handler_destroy;

    return handler;
}

// ========== DisconnectHandler实现 ==========

namespace details {

static int disconnect_handler_handle_request(DebugHandler* self,
                                             const ClientContext* ctx,
                                             const DebugConfig* config,
                                             DebugContext* debug_ctx)
{
    (void)self;
    (void)ctx;

    if (config == nullptr || debug_ctx == nullptr) {
        return kRetInvalidParam;
    }

    if (config->force_disconnect) {
        debug_ctx->disconnect_after = true;
        return kRetStopChain;
    }
    return kRetSuccess;
}

static int disconnect_handler_handle_response(DebugHandler* self,
                                              const ClientContext* ctx,
                                              const DebugConfig* config,
                                              DebugContext* debug_ctx)
{
    (void)self;
    (void)ctx;

    if (config == nullptr || debug_ctx == nullptr) {
        return kRetInvalidParam;
    }

    if (config->force_disconnect) {
        debug_ctx->disconnect_after = true;
        return kRetStopChain;
    }
    return kRetSuccess;
}

static void disconnect_handler_destroy(DebugHandler* self)
{
    if (self != nullptr) {
        std::free(self);
    }
}

} // namespace details

DebugHandler* create_disconnect_handler()
{
    DebugHandler* handler = (DebugHandler*)std::malloc(sizeof(DebugHandler));
    if (handler == nullptr) {
        return nullptr;
    }

    handler->name = kDisconnectHandlerName;
    handler->priority = kDisconnectHandlerPriority;
    handler->user_data = nullptr;
    handler->handle_request = details::disconnect_handler_handle_request;
    handler->handle_response = details::disconnect_handler_handle_response;
    handler->destroy = details::disconnect_handler_destroy;

    return handler;
}

// ========== LogHandler实现 ==========

namespace details {

static int log_handler_handle_request(DebugHandler* self,
                                      const ClientContext* ctx,
                                      const DebugConfig* config,
                                      DebugContext* debug_ctx)
{
    (void)self;
    (void)debug_ctx;

    if (config == nullptr || ctx == nullptr) {
        return kRetInvalidParam;
    }

    if (config->log_packet) {
        auto& logger = utils::Logger::instance();
        logger.info(kLoggerName,
                    "[Debug] Request: conn_id=%llu, client=%s:%u, server_port=%u",
                    (unsigned long long)ctx->connection_id,
                    ctx->client_ip ? ctx->client_ip : "unknown",
                    (unsigned int)ctx->client_port,
                    (unsigned int)ctx->server_port);
    }
    return kRetSuccess;
}

static int log_handler_handle_response(DebugHandler* self,
                                       const ClientContext* ctx,
                                       const DebugConfig* config,
                                       DebugContext* debug_ctx)
{
    (void)self;
    (void)debug_ctx;

    if (config == nullptr || ctx == nullptr) {
        return kRetInvalidParam;
    }

    if (config->log_packet) {
        auto& logger = utils::Logger::instance();
        logger.info(kLoggerName,
                    "[Debug] Response: conn_id=%llu, client=%s:%u, server_port=%u",
                    (unsigned long long)ctx->connection_id,
                    ctx->client_ip ? ctx->client_ip : "unknown",
                    (unsigned int)ctx->client_port,
                    (unsigned int)ctx->server_port);
    }
    return kRetSuccess;
}

static void log_handler_destroy(DebugHandler* self)
{
    if (self != nullptr) {
        std::free(self);
    }
}

} // namespace details

DebugHandler* create_log_handler()
{
    DebugHandler* handler = (DebugHandler*)std::malloc(sizeof(DebugHandler));
    if (handler == nullptr) {
        return nullptr;
    }

    handler->name = kLogHandlerName;
    handler->priority = kLogHandlerPriority;
    handler->user_data = nullptr;
    handler->handle_request = details::log_handler_handle_request;
    handler->handle_response = details::log_handler_handle_response;
    handler->destroy = details::log_handler_destroy;

    return handler;
}

// ========== ErrorCodeHandler实现 ==========

namespace details {

static int errorcode_handler_handle_request(DebugHandler* self,
                                             const ClientContext* ctx,
                                             const DebugConfig* config,
                                             DebugContext* debug_ctx)
{
    (void)self;
    (void)ctx;

    if (config == nullptr || debug_ctx == nullptr) {
        return kRetInvalidParam;
    }

    debug_ctx->override_http_status = config->http_status;
    return kRetSuccess;
}

static int errorcode_handler_handle_response(DebugHandler* self,
                                              const ClientContext* ctx,
                                              const DebugConfig* config,
                                              DebugContext* debug_ctx)
{
    (void)self;
    (void)ctx;

    if (config == nullptr || debug_ctx == nullptr) {
        return kRetInvalidParam;
    }

    if (debug_ctx->override_http_status == 0) {
        debug_ctx->override_http_status = config->http_status;
    }
    return kRetSuccess;
}

static void errorcode_handler_destroy(DebugHandler* self)
{
    if (self != nullptr) {
        std::free(self);
    }
}

} // namespace details

DebugHandler* create_errorcode_handler()
{
    DebugHandler* handler = (DebugHandler*)std::malloc(sizeof(DebugHandler));
    if (handler == nullptr) {
        return nullptr;
    }

    handler->name = kErrorCodeHandlerName;
    handler->priority = kErrorCodeHandlerPriority;
    handler->user_data = nullptr;
    handler->handle_request = details::errorcode_handler_handle_request;
    handler->handle_response = details::errorcode_handler_handle_response;
    handler->destroy = details::errorcode_handler_destroy;

    return handler;
}

} // namespace debug_chain
} // namespace https_server_sim

// ========== DebugHandlerRegistry C接口实现 ==========

// DebugHandlerRegistry结构体定义（内部使用）
struct DebugHandlerRegistry {
    https_server_sim::debug_chain::DebugChain* chain;
};

DebugHandlerRegistry* debug_handler_registry_create(void)
{
    using namespace https_server_sim::debug_chain;

    DebugHandlerRegistry* registry = new DebugHandlerRegistry();
    if (registry == nullptr) {
        return nullptr;
    }

    registry->chain = new DebugChain();
    if (registry->chain == nullptr) {
        delete registry;
        return nullptr;
    }

    // 自动注册4个默认Handler
    registry->chain->register_handler(create_delay_handler());
    registry->chain->register_handler(create_disconnect_handler());
    registry->chain->register_handler(create_log_handler());
    registry->chain->register_handler(create_errorcode_handler());

    return registry;
}

void debug_handler_registry_destroy(DebugHandlerRegistry* registry)
{
    if (registry == nullptr) {
        return;
    }

    if (registry->chain != nullptr) {
        delete registry->chain;
    }

    delete registry;
}

int debug_handler_registry_register(DebugHandlerRegistry* registry,
                                     void* handler_void)
{
    using namespace https_server_sim::debug_chain;

    if (registry == nullptr || registry->chain == nullptr || handler_void == nullptr) {
        return kRetInvalidParam;
    }

    DebugHandler* handler = static_cast<DebugHandler*>(handler_void);

    if (handler->name == nullptr) {
        return kRetInvalidParam;
    }

    if (registry->chain->has_handler(handler->name)) {
        return kRetInvalidParam;
    }

    return registry->chain->register_handler(handler);
}

int debug_handler_registry_unregister(DebugHandlerRegistry* registry,
                                       const char* name)
{
    using namespace https_server_sim::debug_chain;

    if (registry == nullptr || registry->chain == nullptr) {
        return kRetInvalidParam;
    }

    return registry->chain->unregister_handler(name);
}

void* debug_handler_registry_get_chain(DebugHandlerRegistry* registry)
{
    if (registry == nullptr) {
        return nullptr;
    }
    return registry->chain;
}
