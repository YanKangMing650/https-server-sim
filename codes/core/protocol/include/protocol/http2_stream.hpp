// =============================================================================
//  HTTPS Server Simulator - Protocol Module
//  文件: http2_stream.hpp
//  描述: Http2Stream类定义
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include "protocol/protocol_types.hpp"
#include "protocol/http_message.hpp"
#include <cstdint>

namespace https_server_sim {
namespace protocol {

// ==================== HTTP/2流类 ====================
class Http2Stream {
public:
    /**
     * @brief 默认构造函数
     */
    Http2Stream();

    /**
     * @brief 析构函数
     */
    ~Http2Stream();

    /**
     * @brief 初始化流
     * @param stream_id 流ID
     */
    void init(uint32_t stream_id);

    /**
     * @brief 重置流到初始状态
     */
    void reset();

    // 公开属性
    uint32_t stream_id;
    Http2StreamState state;
    HttpRequest request;
    HttpResponse response;
    int32_t send_window;
    int32_t recv_window;
};

} // namespace protocol
} // namespace https_server_sim

// 文件结束
