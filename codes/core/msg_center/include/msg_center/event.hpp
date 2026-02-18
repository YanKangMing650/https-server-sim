// =============================================================================
//  HTTPS Server Simulator - MsgCenter Module
//  文件: event.hpp
//  描述: Event结构、EventType枚举和MsgCenterError错误码定义
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include <cstdint>
#include <functional>

namespace https_server_sim {

// 事件类型枚举
enum class EventType : uint8_t {
    SHUTDOWN = 0,
    ERROR = 1,
    ACCEPT = 2,
    READ = 3,
    WRITE = 4,
    CALLBACK_DONE = 5,
    TIMEOUT = 6,
    STATISTICS = 7
};

// Event优先级数量常量，基于EventType枚举自动计算
constexpr size_t kEventPriorityCount = static_cast<size_t>(EventType::STATISTICS) + 1;

// 错误码枚举
enum class MsgCenterError : int {
    SUCCESS = 0,
    ALREADY_RUNNING = 1,
    THREAD_CREATE_FAILED = 2,
    INVALID_PARAMETER = 3
};

// 事件结构
struct Event {
    EventType type;
    uint64_t conn_id;
    int fd;
    void* user_data;
    std::function<void()> handler;

    /**
     * @brief 创建SHUTDOWN事件
     * @return SHUTDOWN事件
     */
    static Event make_shutdown_event() {
        Event event;
        event.type = EventType::SHUTDOWN;
        event.conn_id = 0;
        event.fd = -1;
        event.user_data = nullptr;
        event.handler = nullptr;
        return event;
    }

    /**
     * @brief 创建CALLBACK_DONE事件
     * @return CALLBACK_DONE事件
     */
    static Event make_callback_done_event() {
        Event event;
        event.type = EventType::CALLBACK_DONE;
        event.conn_id = 0;
        event.fd = -1;
        event.user_data = nullptr;
        event.handler = nullptr;
        return event;
    }
};

} // namespace https_server_sim

// 文件结束
