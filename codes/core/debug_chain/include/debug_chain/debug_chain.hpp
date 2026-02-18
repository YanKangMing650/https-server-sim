#pragma once

#include <vector>
#include <atomic>
#include "debug_chain/debug_config.hpp"
#include "debug_chain/debug_context.hpp"
#include "debug_chain/debug_handler.hpp"
#include "callback/client_context.h"

namespace https_server_sim {
namespace debug_chain {

// DebugChain类 - 职责链管理器
class DebugChain {
public:
    // 返回值常量（明确语义）
    static constexpr int kRetSuccess = 0;           // 操作成功
    static constexpr int kRetInvalidParam = -1;      // 参数无效
    static constexpr int kRetNotFound = -2;           // 未找到
    static constexpr int kRetAlreadyExists = -3;      // 已存在

    // 构造函数
    DebugChain();

    // 析构函数：遍历handlers_，对非空destroy指针的handler调用destroy()
    ~DebugChain();

    // 注册调测点
    // handler: 调测点指针
    // return: kRetSuccess=成功, kRetInvalidParam=参数无效, kRetAlreadyExists=已存在
    int register_handler(DebugHandler* handler);

    // 注销调测点
    // name: 调测点名称
    // return: kRetSuccess=成功, kRetInvalidParam=参数无效, kRetNotFound=未找到
    int unregister_handler(const char* name);

    // 检查是否已存在同名handler
    // name: 调测点名称
    // return: 存在返回true，否则返回false
    bool has_handler(const char* name) const;

    // 处理请求
    // ctx: 客户端上下文
    // config: 调测配置
    // debug_ctx: 调测上下文
    // return: 0继续处理，非0终止链
    int process_request(const ClientContext* ctx,
                       const DebugConfig* config,
                       DebugContext* debug_ctx);

    // 处理响应
    // ctx: 客户端上下文
    // config: 调测上下文
    // debug_ctx: 调测上下文
    // return: 0继续处理，非0终止链
    int process_response(const ClientContext* ctx,
                        const DebugConfig* config,
                        DebugContext* debug_ctx);

private:

    // 按priority升序排序调测点
    void sort_handlers();

    std::vector<DebugHandler*> handlers_;  // 调测点列表
    std::atomic<bool> sorted_;          // 是否已排序（使用atomic避免数据竞争）
};

} // namespace debug_chain
} // namespace https_server_sim

// ========== DebugHandlerRegistry C接口 ==========

#ifdef __cplusplus
extern "C" {
#endif

// DebugHandlerRegistry前置声明
typedef struct DebugHandlerRegistry DebugHandlerRegistry;

// 创建DebugHandlerRegistry实例
// 内部自动注册4个默认Handler（DelayHandler、DisconnectHandler、LogHandler、ErrorCodeHandler）
DebugHandlerRegistry* debug_handler_registry_create(void);

// 销毁DebugHandlerRegistry实例
// 内部调用所有已注册Handler的destroy()函数
void debug_handler_registry_destroy(DebugHandlerRegistry* registry);

// 注册调测点
// registry: 注册表实例
// handler: 调测点指针（DebugHandler*，与设计文档一致）
// return: 0=成功, -1=参数无效, -3=已存在
int debug_handler_registry_register(DebugHandlerRegistry* registry,
                                     DebugHandler* handler);

// 注销调测点
// registry: 注册表实例
// name: 调测点名称
// return: 0=成功, -1=参数无效, -2=未找到
int debug_handler_registry_unregister(DebugHandlerRegistry* registry,
                                       const char* name);

// 获取内部DebugChain实例（类型安全版本）
// registry: 注册表实例
// return: DebugChain指针，失败返回NULL
https_server_sim::debug_chain::DebugChain* debug_handler_registry_get_chain(DebugHandlerRegistry* registry);

#ifdef __cplusplus
}
#endif

// 模块：DebugChain - 职责链管理器
// 用途：管理调测点处理器的注册、注销和执行
