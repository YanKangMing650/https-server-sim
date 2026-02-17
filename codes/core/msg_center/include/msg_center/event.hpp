#pragma once

#include <cstdint>
#include <functional>
#include <memory>

namespace https_server_sim {
namespace msg_center {

// 事件类型
enum class EventType : uint8_t {
    ACCEPT,
    READ,
    WRITE,
    CLOSE,
    TIMEOUT,
    CUSTOM
};

// 事件回调类型
using EventCallback = std::function<void()>;

// 事件类
class Event {
public:
    Event();
    explicit Event(EventType type);
    Event(EventType type, EventCallback callback);
    ~Event();

    // 获取事件类型
    EventType get_type() const;
    void set_type(EventType type);

    // 获取/设置回调
    const EventCallback& get_callback() const;
    void set_callback(EventCallback callback);

    // 执行回调
    void execute();

private:
    EventType type_;
    EventCallback callback_;
};

} // namespace msg_center
} // namespace https_server_sim
