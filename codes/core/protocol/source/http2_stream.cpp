// =============================================================================
//  HTTPS Server Simulator - Protocol Module
//  文件: http2_stream.cpp
//  描述: Http2Stream类实现
//  版权: Copyright (c) 2026
// =============================================================================
#include "protocol/http2_stream.hpp"

namespace https_server_sim {
namespace protocol {

// ==================== Http2Stream实现 ====================

Http2Stream::Http2Stream()
    : stream_id(0)
    , state(Http2StreamState::IDLE)
    , request()
    , response()
    , send_window(HTTP2_DEFAULT_INITIAL_WINDOW_SIZE)
    , recv_window(HTTP2_DEFAULT_INITIAL_WINDOW_SIZE)
{
}

Http2Stream::~Http2Stream() = default;

void Http2Stream::init(uint32_t id) {
    stream_id = id;
    state = Http2StreamState::IDLE;
    request.reset();
    response.reset();
    send_window = HTTP2_DEFAULT_INITIAL_WINDOW_SIZE;
    recv_window = HTTP2_DEFAULT_INITIAL_WINDOW_SIZE;
}

void Http2Stream::reset() {
    stream_id = 0;
    state = Http2StreamState::IDLE;
    request.reset();
    response.reset();
    send_window = HTTP2_DEFAULT_INITIAL_WINDOW_SIZE;
    recv_window = HTTP2_DEFAULT_INITIAL_WINDOW_SIZE;
}

} // namespace protocol
} // namespace https_server_sim

// 文件结束
