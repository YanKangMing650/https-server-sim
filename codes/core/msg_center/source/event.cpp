#include "msg_center/event.hpp"

namespace https_server_sim {
namespace msg_center {

Event::Event()
    : type_(EventType::CUSTOM)
{
}

Event::Event(EventType type)
    : type_(type)
{
}

Event::Event(EventType type, EventCallback callback)
    : type_(type)
    , callback_(std::move(callback))
{
}

Event::~Event() {
}

EventType Event::get_type() const {
    return type_;
}

void Event::set_type(EventType type) {
    type_ = type;
}

const EventCallback& Event::get_callback() const {
    return callback_;
}

void Event::set_callback(EventCallback callback) {
    callback_ = std::move(callback);
}

void Event::execute() {
    if (callback_) {
        callback_();
    }
}

} // namespace msg_center
} // namespace https_server_sim
