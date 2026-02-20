// HTTPS Server 模拟器 - DebugChain实现文件
// 模块：DebugChain - 职责链管理器
// 用途：实现DebugChain类和默认Handler

#include "debug_chain/debug_chain.hpp"
#include "debug_chain/debug_handler.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <cinttypes>
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

// 日志模块名称
const char* const kLoggerName = "DebugChain";

} // anonymous namespace

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
        return ERR_INVALID_PARAM;
    }
    if (handler->name == nullptr) {
        return ERR_INVALID_PARAM;
    }
    if (has_handler(handler->name)) {
        return ERR_ALREADY_EXISTS;
    }
    handlers_.push_back(handler);
    sorted_ = false;
    return ERR_SUCCESS;
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
        return ERR_INVALID_PARAM;
    }

    auto it = std::find_if(handlers_.begin(), handlers_.end(),
        [name](DebugHandler* handler) {
            return handler != nullptr && handler->name != nullptr &&
                   std::strcmp(handler->name, name) == 0;
        });

    if (it == handlers_.end()) {
        return ERR_NOT_FOUND;
    }

    // 调用destroy释放handler内存
    DebugHandler* handler = *it;
    if (handler->destroy != nullptr) {
        handler->destroy(handler);
    }

    handlers_.erase(it);
    return ERR_SUCCESS;
}

// 私有模板方法：处理请求/响应的公共逻辑
template<typename HandlerFunc>
int DebugChain::process_internal(const ClientContext* ctx,
                                  const DebugConfig* config,
                                  DebugContext* debug_ctx,
                                  HandlerFunc handler_func)
{
    // Debug模式下检查空指针参数（便于发现调用方错误）
    assert(ctx != nullptr && "process: ctx parameter is null");
    assert(config != nullptr && "process: config parameter is null");
    assert(debug_ctx != nullptr && "process: debug_ctx parameter is null");

    if (ctx == nullptr || config == nullptr || debug_ctx == nullptr) {
        return ERR_INVALID_PARAM;
    }

    if (!config->enabled) {
        return ERR_NOT_EXECUTED;
    }

    if (!sorted_) {
        sort_handlers();
    }

    for (auto handler : handlers_) {
        if (handler == nullptr) {
            continue;
        }

        int result = handler_func(handler, ctx, config, debug_ctx);
        if (result != ERR_CONTINUE_CHAIN) {
            return result;
        }
    }

    return ERR_CONTINUE_CHAIN;
}

int DebugChain::process_request(const ClientContext* ctx,
                                const DebugConfig* config,
                                DebugContext* debug_ctx)
{
    return process_internal(ctx, config, debug_ctx,
        [](DebugHandler* handler, const ClientContext* c, const DebugConfig* cfg, DebugContext* dc) {
            if (handler->handle_request == nullptr) {
                return ERR_CONTINUE_CHAIN;
            }
            return handler->handle_request(handler, c, cfg, dc);
        });
}

int DebugChain::process_response(const ClientContext* ctx,
                                 const DebugConfig* config,
                                 DebugContext* debug_ctx)
{
    return process_internal(ctx, config, debug_ctx,
        [](DebugHandler* handler, const ClientContext* c, const DebugConfig* cfg, DebugContext* dc) {
            if (handler->handle_response == nullptr) {
                return ERR_CONTINUE_CHAIN;
            }
            return handler->handle_response(handler, c, cfg, dc);
        });
}

void DebugChain::sort_handlers()
{
    // 按priority升序排序（register_handler已保证handler和name非空）
    std::sort(handlers_.begin(), handlers_.end(),
        [](DebugHandler* a, DebugHandler* b) {
            // 先按priority升序排序
            if (a->priority != b->priority) {
                return a->priority < b->priority;
            }
            // priority相同，按名称字符串排序确保可预测的顺序
            return std::strcmp(a->name, b->name) < 0;
        });
    sorted_ = true;
}

// ========== DelayHandler实现 ==========

namespace details {

// 公共延迟逻辑函数（消除重复代码）
static void delay_common(const DebugConfig* config)
{
    if (config != nullptr && config->delay_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(config->delay_ms));
    }
}

static int delay_handler_handle_request(DebugHandler* self,
                                        const ClientContext* ctx,
                                        const DebugConfig* config,
                                        DebugContext* debug_ctx)
{
    (void)self;
    (void)ctx;
    (void)debug_ctx;

    if (config == nullptr) {
        return DebugChain::ERR_CONTINUE_CHAIN;
    }

    delay_common(config);
    return DebugChain::ERR_CONTINUE_CHAIN;
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
        return DebugChain::ERR_CONTINUE_CHAIN;
    }

    delay_common(config);
    return DebugChain::ERR_CONTINUE_CHAIN;
}

static void delay_handler_destroy(DebugHandler* self)
{
    if (self != nullptr) {
        delete self;
    }
}

} // namespace details

DebugHandler* create_delay_handler()
{
    DebugHandler* handler = new DebugHandler();

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

// 公共断开连接逻辑函数（消除重复代码）
static int disconnect_common(const DebugConfig* config, DebugContext* debug_ctx)
{
    if (config == nullptr || debug_ctx == nullptr) {
        return DebugChain::ERR_CONTINUE_CHAIN;
    }

    if (config->force_disconnect) {
        debug_ctx->disconnect_after = true;
        return DebugChain::ERR_STOP_CHAIN;
    }
    return DebugChain::ERR_CONTINUE_CHAIN;
}

static int disconnect_handler_handle_request(DebugHandler* self,
                                             const ClientContext* ctx,
                                             const DebugConfig* config,
                                             DebugContext* debug_ctx)
{
    (void)self;
    (void)ctx;

    return disconnect_common(config, debug_ctx);
}

static int disconnect_handler_handle_response(DebugHandler* self,
                                              const ClientContext* ctx,
                                              const DebugConfig* config,
                                              DebugContext* debug_ctx)
{
    (void)self;
    (void)ctx;

    return disconnect_common(config, debug_ctx);
}

static void disconnect_handler_destroy(DebugHandler* self)
{
    if (self != nullptr) {
        delete self;
    }
}

} // namespace details

DebugHandler* create_disconnect_handler()
{
    DebugHandler* handler = new DebugHandler();

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

// 公共日志逻辑函数（消除重复代码）
static void log_common(const ClientContext* ctx, const char* direction)
{
    if (ctx == nullptr || direction == nullptr) {
        return;
    }

    auto& logger = utils::Logger::instance();
    logger.info(kLoggerName,
                "[Debug] %s: conn_id=%" PRIu64 ", client=%s:%" PRIu16 ", server_port=%" PRIu16,
                direction,
                ctx->connection_id,
                ctx->client_ip ? ctx->client_ip : "unknown",
                (uint16_t)ctx->client_port,
                (uint16_t)ctx->server_port);
}

static int log_handler_handle_request(DebugHandler* self,
                                      const ClientContext* ctx,
                                      const DebugConfig* config,
                                      DebugContext* debug_ctx)
{
    (void)self;
    (void)debug_ctx;

    if (config == nullptr || ctx == nullptr) {
        return DebugChain::ERR_CONTINUE_CHAIN;
    }

    if (config->log_packet) {
        log_common(ctx, "Request");
    }
    return DebugChain::ERR_CONTINUE_CHAIN;
}

static int log_handler_handle_response(DebugHandler* self,
                                       const ClientContext* ctx,
                                       const DebugConfig* config,
                                       DebugContext* debug_ctx)
{
    (void)self;
    (void)debug_ctx;

    if (config == nullptr || ctx == nullptr) {
        return DebugChain::ERR_CONTINUE_CHAIN;
    }

    if (config->log_packet) {
        log_common(ctx, "Response");
    }
    return DebugChain::ERR_CONTINUE_CHAIN;
}

static void log_handler_destroy(DebugHandler* self)
{
    if (self != nullptr) {
        delete self;
    }
}

} // namespace details

DebugHandler* create_log_handler()
{
    DebugHandler* handler = new DebugHandler();

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
        return DebugChain::ERR_CONTINUE_CHAIN;
    }

    debug_ctx->override_http_status = config->http_status;
    return DebugChain::ERR_CONTINUE_CHAIN;
}

static int errorcode_handler_handle_response(DebugHandler* self,
                                              const ClientContext* ctx,
                                              const DebugConfig* config,
                                              DebugContext* debug_ctx)
{
    (void)self;
    (void)ctx;

    if (config == nullptr || debug_ctx == nullptr) {
        return DebugChain::ERR_CONTINUE_CHAIN;
    }

    if (debug_ctx->override_http_status == 0) {
        debug_ctx->override_http_status = config->http_status;
    }
    return DebugChain::ERR_CONTINUE_CHAIN;
}

static void errorcode_handler_destroy(DebugHandler* self)
{
    if (self != nullptr) {
        delete self;
    }
}

} // namespace details

DebugHandler* create_errorcode_handler()
{
    DebugHandler* handler = new DebugHandler();

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

DebugHandlerRegistry* debug_handler_registry_create_empty(void)
{
    using namespace https_server_sim::debug_chain;

    DebugHandlerRegistry* registry = new DebugHandlerRegistry();
    registry->chain = new DebugChain();

    return registry;
}

DebugHandlerRegistry* debug_handler_registry_create(void)
{
    using namespace https_server_sim::debug_chain;

    // 使用空创建函数，然后手动注册默认handler
    DebugHandlerRegistry* registry = debug_handler_registry_create_empty();

    // 【行为说明】：自动注册默认handler是设计行为，方便快速使用
    // 若不需要自动注册，请使用 debug_handler_registry_create_empty()
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
                                     DebugHandler* handler)
{
    using namespace https_server_sim::debug_chain;

    if (registry == nullptr || registry->chain == nullptr || handler == nullptr) {
        return DEBUG_HANDLER_REGISTRY_INVALID_PARAM;
    }

    if (handler->name == nullptr) {
        return DEBUG_HANDLER_REGISTRY_INVALID_PARAM;
    }

    // 至少需要一个处理函数（handle_request 或 handle_response）
    if (handler->handle_request == nullptr && handler->handle_response == nullptr) {
        return DEBUG_HANDLER_REGISTRY_INVALID_PARAM;
    }

    if (registry->chain->has_handler(handler->name)) {
        return DEBUG_HANDLER_REGISTRY_ALREADY_EXISTS;
    }

    // 数值一致，直接返回
    return registry->chain->register_handler(handler);
}

int debug_handler_registry_unregister(DebugHandlerRegistry* registry,
                                       const char* name)
{
    using namespace https_server_sim::debug_chain;

    if (registry == nullptr || registry->chain == nullptr) {
        return DEBUG_HANDLER_REGISTRY_INVALID_PARAM;
    }

    // 数值一致，直接返回
    return registry->chain->unregister_handler(name);
}

https_server_sim::debug_chain::DebugChain* debug_handler_registry_get_chain(DebugHandlerRegistry* registry)
{
    if (registry == nullptr) {
        return nullptr;
    }
    return registry->chain;
}
