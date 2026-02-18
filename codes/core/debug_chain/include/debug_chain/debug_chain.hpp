// HTTPS Server 模拟器 - DebugChain头文件
// 模块：DebugChain - 职责链管理器
// 用途：管理调测点处理器的注册、注销和执行
// 重要使用约定：
//   - 注册/注销操作必须在process_request/process_response调用前完成
//   - 注册/注销与执行操作不能并发调用
//   - 本类不保证线程安全，调用方需保证上述约定
// 所有权说明（关键）：
//   - register_handler调用后，handler的所有权转移给DebugChain
//   - unregister_handler或DebugChain析构时，会通过handler->destroy释放资源
// 返回值风格说明：
//   - C++接口使用constexpr常量，C接口使用宏定义
//   - 两者数值保持一致，这是为了C兼容性而特意设计的

#pragma once

#include <vector>
#include "debug_chain/debug_config.hpp"
#include "debug_chain/debug_context.hpp"
#include "debug_chain/debug_handler.hpp"
#include "callback/client_context.h"

namespace https_server_sim {
namespace debug_chain {

// DebugChain类 - 职责链管理器
// 线程安全约定：
//   - register_handler/unregister_handler仅在Server启动前调用
//   - process_request/process_response在Server运行期间调用
//   - 调用方必须保证注册/注销与执行操作不并发
class DebugChain {
public:
    // ========== C++接口返回值常量（constexpr版本） ==========
    static constexpr int kRetSuccess = 0;           // 操作成功
    static constexpr int kRetInvalidParam = -1;      // 参数无效
    static constexpr int kRetNotFound = -2;           // 未找到
    static constexpr int kRetAlreadyExists = -3;      // 已存在
    static constexpr int kRetContinueChain = 0;       // 继续执行链
    static constexpr int kRetStopChain = 1;           // 终止链执行

    // 构造函数
    DebugChain();

    // 析构函数：遍历handlers_，对非空destroy指针的handler调用destroy()
    ~DebugChain();

    // 注册调测点
    // handler: 调测点指针（不能为空，handler->name不能为空）
    // return: kRetSuccess=成功, kRetInvalidParam=参数无效, kRetAlreadyExists=已存在
    // 【所有权转移说明】：调用register_handler后，handler的所有权完全转移给DebugChain，
    //                     调用方不再拥有handler的所有权，不得在外部释放handler
    //                     DebugChain会在析构或unregister_handler时通过handler->destroy释放资源
    int register_handler(DebugHandler* handler);

    // 注销调测点
    // name: 调测点名称
    // return: kRetSuccess=成功, kRetInvalidParam=参数无效, kRetNotFound=未找到
    // 【所有权转移说明】：unregister_handler会将handler的所有权从DebugChain中移除，
    //                     并立即调用handler->destroy释放资源
    int unregister_handler(const char* name);

    // 检查是否已存在同名handler
    // name: 调测点名称
    // return: 存在返回true，否则返回false
    bool has_handler(const char* name) const;

    // 处理请求
    // ctx: 客户端上下文
    // config: 调测配置
    // debug_ctx: 调测上下文
    // return: kRetContinueChain=继续处理，其他值=终止链
    int process_request(const ClientContext* ctx,
                       const DebugConfig* config,
                       DebugContext* debug_ctx);

    // 处理响应
    // ctx: 客户端上下文
    // config: 调测配置
    // debug_ctx: 调测上下文
    // return: kRetContinueChain=继续处理，其他值=终止链
    int process_response(const ClientContext* ctx,
                        const DebugConfig* config,
                        DebugContext* debug_ctx);

private:
    // 按priority升序排序调测点
    void sort_handlers();

    std::vector<DebugHandler*> handlers_;  // 调测点列表
    bool sorted_;                           // 是否已排序
};

} // namespace debug_chain
} // namespace https_server_sim

// ========== DebugHandlerRegistry C接口 ==========

#ifdef __cplusplus
extern "C" {
#endif

// DebugHandlerRegistry前置声明
typedef struct DebugHandlerRegistry DebugHandlerRegistry;

// ========== C接口返回值常量（宏定义版本 - 为C兼容性特意设计） ==========
// 注意：C接口使用宏定义是为了兼容纯C代码
//       所有宏值与C++接口的constexpr常量数值保持完全一致
//       这种风格差异是为了同时支持C和C++而特意设计的
#define DEBUG_HANDLER_REGISTRY_SUCCESS        0   // 对应kRetSuccess
#define DEBUG_HANDLER_REGISTRY_INVALID_PARAM  -1  // 对应kRetInvalidParam
#define DEBUG_HANDLER_REGISTRY_NOT_FOUND      -2  // 对应kRetNotFound
#define DEBUG_HANDLER_REGISTRY_ALREADY_EXISTS -3  // 对应kRetAlreadyExists

// 创建DebugHandlerRegistry实例
// 内部自动注册4个默认Handler（DelayHandler、DisconnectHandler、LogHandler、ErrorCodeHandler）
DebugHandlerRegistry* debug_handler_registry_create(void);

// 销毁DebugHandlerRegistry实例
// 内部调用所有已注册Handler的destroy()函数
void debug_handler_registry_destroy(DebugHandlerRegistry* registry);

// 注册调测点
// registry: 注册表实例
// handler: 调测点指针
// return: DEBUG_HANDLER_REGISTRY_SUCCESS=成功,
//         DEBUG_HANDLER_REGISTRY_INVALID_PARAM=参数无效,
//         DEBUG_HANDLER_REGISTRY_ALREADY_EXISTS=已存在
// 【所有权转移说明】：调用debug_handler_registry_register后，handler的所有权完全转移给registry，
//                     调用方不再拥有handler的所有权，不得在外部释放handler
//                     registry会在destroy或unregister时通过handler->destroy释放资源
int debug_handler_registry_register(DebugHandlerRegistry* registry,
                                     DebugHandler* handler);

// 注销调测点
// registry: 注册表实例
// name: 调测点名称
// return: DEBUG_HANDLER_REGISTRY_SUCCESS=成功,
//         DEBUG_HANDLER_REGISTRY_INVALID_PARAM=参数无效,
//         DEBUG_HANDLER_REGISTRY_NOT_FOUND=未找到
// 【所有权转移说明】：debug_handler_registry_unregister会将handler的所有权从registry中移除，
//                     并立即调用handler->destroy释放资源
int debug_handler_registry_unregister(DebugHandlerRegistry* registry,
                                       const char* name);

// 获取内部DebugChain实例（类型安全版本，仅C++使用）
// registry: 注册表实例
// return: DebugChain指针，失败返回NULL
https_server_sim::debug_chain::DebugChain* debug_handler_registry_get_chain(DebugHandlerRegistry* registry);

#ifdef __cplusplus
}
#endif

// 模块：DebugChain - 职责链管理器
// 用途：管理调测点处理器的注册、注销和执行
