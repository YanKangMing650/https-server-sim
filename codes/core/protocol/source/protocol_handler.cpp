// =============================================================================
//  HTTPS Server Simulator - Protocol Module
//  文件: protocol_handler.cpp
//  描述: ProtocolHandler接口实现类（Http1Handler/Http2Handler/ProtocolFactory）
//  版权: Copyright (c) 2026
// =============================================================================
#include "protocol/protocol_handler.hpp"
#include "protocol/protocol_factory.hpp"
#include "protocol/http_parser.hpp"
#include "protocol/protocol_utils.hpp"
#include "connection/connection.hpp"
#include "callback/client_context.h"
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cerrno>

namespace https_server_sim {
namespace protocol {

// ==================== Http1Handler实现 ====================

Http1Handler::Http1Handler()
    : conn_(nullptr)
    , tls_handler_(std::make_unique<TlsHandler>())
    , state_(Http1ParseState::EXPECT_REQUEST_LINE)
    , request_()
    , response_()
    , read_buffer_(nullptr)
    , write_buffer_(nullptr)
    , plaintext_buffer_(std::make_unique<utils::Buffer>())
    , parser_()
{
}

Http1Handler::~Http1Handler() = default;

int Http1Handler::init(Connection* conn,
                       const CertConfig& cert_config,
                       const TlsConfig& tls_config) {
    conn_ = conn;

    int ret = tls_handler_->init(conn, cert_config, tls_config);
    if (ret != PROTOCOL_OK) {
        return ret;
    }

    // 从Connection获取Buffer指针
    if (conn_) {
        read_buffer_ = &conn_->get_read_buffer();
        write_buffer_ = &conn_->get_write_buffer();
        // 设置TlsHandler的Buffer指针
        tls_handler_->set_read_buffer(read_buffer_);
        tls_handler_->set_write_buffer(write_buffer_);
    }

    reset();
    return PROTOCOL_OK;
}

int Http1Handler::on_read() {
    if (!tls_handler_) {
        return PROTOCOL_ERROR_INVALID;
    }

    uint8_t temp_buf[TEMP_BUFFER_SIZE];
    size_t read_len = 0;
    int ret = tls_handler_->read(temp_buf, sizeof(temp_buf), &read_len);
    if (ret == PROTOCOL_ERROR_EAGAIN) {
        return PROTOCOL_OK;
    }
    if (ret < 0) {
        state_ = Http1ParseState::ERROR;
        response_.status_code = 400;
        response_.status_text = "Bad Request";
        generate_response();
        return PROTOCOL_ERROR_INVALID;
    }

    if (read_len > 0) {
        plaintext_buffer_->write(temp_buf, read_len);
    }

    // 确保parser_已正确初始化
    parser_.init(plaintext_buffer_.get());

    while (true) {
        switch (state_) {
            case Http1ParseState::EXPECT_REQUEST_LINE: {
                char line[MAX_LINE_LEN];
                size_t line_len = 0;
                ret = parser_.read_line(line, sizeof(line), &line_len);
                if (ret == PROTOCOL_ERROR_EAGAIN) {
                    return PROTOCOL_OK;
                }
                if (ret < 0) {
                    state_ = Http1ParseState::ERROR;
                    // 直接处理错误状态，避免重复循环
                    response_.status_code = 400;
                    response_.status_text = "Bad Request";
                    generate_response();
                    return PROTOCOL_ERROR_INVALID;
                }
                if (parser_.parse_request_line(line, line_len,
                                               &request_.method,
                                               &request_.path,
                                               &request_.version) < 0) {
                    state_ = Http1ParseState::ERROR;
                    response_.status_code = 400;
                    response_.status_text = "Bad Request";
                    generate_response();
                    return PROTOCOL_ERROR_INVALID;
                }
                state_ = Http1ParseState::EXPECT_HEADERS;
                break;
            }

            case Http1ParseState::EXPECT_HEADERS: {
                ret = parser_.parse_headers(&request_.headers);
                if (ret == PROTOCOL_ERROR_EAGAIN) {
                    return PROTOCOL_OK;
                }
                if (ret < 0) {
                    state_ = Http1ParseState::ERROR;
                    response_.status_code = 400;
                    response_.status_text = "Bad Request";
                    generate_response();
                    return PROTOCOL_ERROR_INVALID;
                }

                std::string te_value;
                if (parser_.find_header(request_.headers, "Transfer-Encoding", &te_value)) {
                    if (te_value.find("chunked") != std::string::npos ||
                        te_value.find("CHUNKED") != std::string::npos) {
                        state_ = Http1ParseState::ERROR;
                        response_.status_code = 400;
                        response_.status_text = "Bad Request";
                        generate_response();
                        return PROTOCOL_ERROR_INVALID;
                    }
                }

                std::string value;
                if (parser_.find_header(request_.headers, "Debug-Token", &value)) {
                    request_.debug_token = value;
                }
                if (parser_.find_header(request_.headers, "Content-Length", &value)) {
                    char* endptr = nullptr;
                    errno = 0;
                    unsigned long long cl = strtoull(value.c_str(), &endptr, 10);
                    if (errno != 0 || endptr == value.c_str() || *endptr != '\0') {
                        state_ = Http1ParseState::ERROR;
                        response_.status_code = 400;
                        response_.status_text = "Bad Request";
                        generate_response();
                        return PROTOCOL_ERROR_INVALID;
                    }
                    request_.content_length = cl;
                } else {
                    request_.content_length = 0;
                }

                if (request_.content_length > 0) {
                    state_ = Http1ParseState::EXPECT_BODY;
                } else {
                    state_ = Http1ParseState::EXPECT_COMPLETE;
                }
                break;
            }

            case Http1ParseState::EXPECT_BODY: {
                size_t remaining = request_.content_length - request_.body.size();
                size_t readable = plaintext_buffer_->readable_bytes();
                size_t to_read = std::min(readable, remaining);
                if (to_read > 0) {
                    const uint8_t* data = plaintext_buffer_->read_ptr();
                    request_.body.insert(request_.body.end(), data, data + to_read);
                    plaintext_buffer_->skip(to_read);
                }
                if (request_.body.size() == request_.content_length) {
                    state_ = Http1ParseState::EXPECT_COMPLETE;
                } else {
                    return PROTOCOL_OK;
                }
                break;
            }

            case Http1ParseState::EXPECT_COMPLETE: {
                ret = handle_complete_request();
                if (ret == PROTOCOL_OK) {
                    reset();
                }
                return ret;
            }

            case Http1ParseState::ERROR: {
                // 这个分支理论上不会到达，因为设置ERROR时已直接返回
                response_.status_code = 400;
                response_.status_text = "Bad Request";
                generate_response();
                return PROTOCOL_ERROR_INVALID;
            }
        }
    }

    return PROTOCOL_OK;
}

int Http1Handler::on_write() {
    // 处理写事件：尝试将TLS层待发送的数据 flush 到Connection写缓冲区
    if (tls_handler_) {
        // TLSHandler的write()方法内部已包含pump_write_bio()逻辑
        // 这里我们无需额外操作，因为generate_response()已通过tls_handler_->write()发送数据
        // 如果有部分写入的场景，这里可以补充处理
    }
    return PROTOCOL_OK;
}

int Http1Handler::send_response(const uint8_t* data, uint32_t len) {
    response_.set_body(data, len);
    return generate_response();
}

void Http1Handler::close() {
    if (tls_handler_) {
        tls_handler_->close();
    }
    reset();
}

TlsHandler* Http1Handler::get_tls_handler() {
    return tls_handler_.get();
}

ProtocolType Http1Handler::get_protocol_type() const {
    return ProtocolType::HTTP_1_1;
}

void Http1Handler::reset() {
    state_ = Http1ParseState::EXPECT_REQUEST_LINE;
    request_.reset();
    response_.reset();
    parser_.reset();
    parser_.init(plaintext_buffer_.get());
}

int Http1Handler::handle_complete_request() {
    // 空指针检查
    if (conn_ == nullptr) {
        return PROTOCOL_ERROR_INVALID;
    }

    // 构建ClientContext并调用Callback模块
    // 注意：将字符串复制到局部变量，确保指针在函数执行期间有效（修复悬空指针问题）
    ClientContext ctx;
    const ClientInfo& client_info = conn_->get_client_info();

    std::string local_client_ip = client_info.client_ip;
    std::string local_debug_token = request_.debug_token;

    ctx.connection_id = client_info.connection_id;
    ctx.client_ip = local_client_ip.c_str();
    ctx.client_port = client_info.client_port;
    ctx.server_port = client_info.server_port;
    ctx.token = local_debug_token.empty() ? nullptr : local_debug_token.c_str();

    // 这里我们使用桩代码模拟Callback模块的调用
    // 实际项目中应该从CallbackRegistry获取策略
    // 现在我们模拟调用回调并生成响应

    // 模拟处理请求体
    const uint8_t* body_data = request_.body.empty() ? nullptr : request_.body.data();
    uint32_t body_len = static_cast<uint32_t>(request_.body.size());

    // 模拟AsyncParseContentFunc调用（桩代码）
    // 实际项目中: strategy->parse(&ctx, body_data, body_len);

    // 模拟AsyncReplyContentFunc调用（桩代码）
    // uint8_t reply_buf[4096];
    // uint32_t reply_len = 0;
    // 实际项目中: strategy->reply(&ctx, reply_buf, &reply_len);

    // 默认回复
    response_.status_code = 200;
    response_.status_text = "OK";
    response_.add_header("Content-Type", "text/plain");

    // 如果有请求体，回显请求体，否则返回"OK"
    if (body_len > 0) {
        response_.set_body(body_data, body_len);
    } else {
        const char* reply = "OK";
        response_.set_body(reinterpret_cast<const uint8_t*>(reply), 2);
    }

    return generate_response();
}

int Http1Handler::generate_response() {
    // 使用动态分配的缓冲区避免栈溢出
    // 先估算需要的大小：状态行 + 头部 + 空行 + 体
    size_t estimated_size = 1024; // 状态行和基本头部
    for (const auto& header : response_.headers) {
        estimated_size += header.first.size() + header.second.size() + 32;
    }
    estimated_size += response_.body.size() + 64; // 额外余量

    std::vector<uint8_t> buf(estimated_size);
    size_t out_len = 0;
    int ret = parser_.build_response(response_, buf.data(), buf.size(), &out_len);

    // 如果缓冲区不够，扩容后重试
    if (ret == PROTOCOL_ERROR_BUFFER) {
        // 计算实际需要的大小
        estimated_size = 4096 + response_.body.size();
        buf.resize(estimated_size);
        ret = parser_.build_response(response_, buf.data(), buf.size(), &out_len);
    }

    if (ret != PROTOCOL_OK) {
        return ret;
    }

    // 只通过TlsHandler进行数据加密发送，避免双重写入
    if (tls_handler_) {
        size_t written = 0;
        ret = tls_handler_->write(buf.data(), out_len, &written);
        if (ret != PROTOCOL_OK) {
            return ret;
        }
        // 检查是否所有数据都写入了
        if (written != out_len) {
            // 部分写入的情况，这里简化处理
            // 实际项目中应该处理这种情况
        }
    }

    return PROTOCOL_OK;
}

// ==================== Http2Handler实现 ====================

Http2Handler::Http2Handler()
    : conn_(nullptr)
    , tls_handler_(std::make_unique<TlsHandler>())
    , streams_()
    , read_buffer_(nullptr)
    , write_buffer_(nullptr)
    , plaintext_buffer_(std::make_unique<utils::Buffer>())
    , settings_()
    , local_settings_()
    , last_stream_id_(0)
    , hpack_encoder_(std::make_unique<HpackEncoder>())
    , hpack_decoder_(std::make_unique<HpackDecoder>())
    , settings_ack_received_(false)
    , settings_ack_sent_(false)
{
}

Http2Handler::~Http2Handler() = default;

int Http2Handler::init(Connection* conn,
                       const CertConfig& cert_config,
                       const TlsConfig& tls_config) {
    conn_ = conn;

    int ret = tls_handler_->init(conn, cert_config, tls_config);
    if (ret != PROTOCOL_OK) {
        return ret;
    }

    // 从Connection获取Buffer指针
    if (conn_) {
        read_buffer_ = &conn_->get_read_buffer();
        write_buffer_ = &conn_->get_write_buffer();
        // 设置TlsHandler的Buffer指针
        tls_handler_->set_read_buffer(read_buffer_);
        tls_handler_->set_write_buffer(write_buffer_);
    }

    settings_.reset();
    local_settings_.reset();
    streams_.clear();
    last_stream_id_ = 0;
    settings_ack_received_ = false;
    settings_ack_sent_ = false;

    send_settings_frame();
    return PROTOCOL_OK;
}

int Http2Handler::on_read() {
    uint8_t temp_buf[TEMP_BUFFER_SIZE];
    size_t read_len = 0;
    int ret = tls_handler_->read(temp_buf, sizeof(temp_buf), &read_len);
    if (ret == PROTOCOL_ERROR_EAGAIN) {
        return PROTOCOL_OK;
    }
    if (ret < 0) {
        return ret;
    }

    if (read_len > 0) {
        plaintext_buffer_->write(temp_buf, read_len);
    }

    while (true) {
        ret = read_frame();
        if (ret == PROTOCOL_ERROR_EAGAIN) {
            break;
        }
        if (ret < 0) {
            return ret;
        }
    }

    return PROTOCOL_OK;
}

int Http2Handler::on_write() {
    // 处理写事件：尝试将TLS层待发送的数据 flush 到Connection写缓冲区
    if (tls_handler_) {
        // TLSHandler的write()方法内部已包含pump_write_bio()逻辑
        // 这里我们无需额外操作
        // 如果有部分写入的场景，这里可以补充处理
    }
    return PROTOCOL_OK;
}

int Http2Handler::send_response(const uint8_t* data, uint32_t len) {
    return send_data_frame(1, data, len, true);
}

void Http2Handler::close() {
    if (tls_handler_) {
        tls_handler_->close();
    }
    reset();
}

TlsHandler* Http2Handler::get_tls_handler() {
    return tls_handler_.get();
}

ProtocolType Http2Handler::get_protocol_type() const {
    return ProtocolType::HTTP_2;
}

void Http2Handler::reset() {
    streams_.clear();
    last_stream_id_ = 0;
    settings_ack_received_ = false;
    settings_ack_sent_ = false;
}

int Http2Handler::read_frame() {
    if (plaintext_buffer_->readable_bytes() < HTTP2_FRAME_HEADER_SIZE) {
        return PROTOCOL_ERROR_EAGAIN;
    }

    const uint8_t* data = plaintext_buffer_->read_ptr();

    Http2FrameHeader header;
    header.length = (static_cast<uint32_t>(data[0]) << 16) |
                   (static_cast<uint32_t>(data[1]) << 8) |
                   static_cast<uint32_t>(data[2]);
    header.type = static_cast<Http2FrameType>(data[3]);
    header.flags = data[4];
    header.stream_id = (static_cast<uint32_t>(data[5]) << 24) |
                      (static_cast<uint32_t>(data[6]) << 16) |
                      (static_cast<uint32_t>(data[7]) << 8) |
                      static_cast<uint32_t>(data[8]);
    header.stream_id &= 0x7FFFFFFF;

    if (plaintext_buffer_->readable_bytes() < HTTP2_FRAME_HEADER_SIZE + header.length) {
        return PROTOCOL_ERROR_EAGAIN;
    }

    plaintext_buffer_->skip(HTTP2_FRAME_HEADER_SIZE);
    const uint8_t* payload = plaintext_buffer_->read_ptr();

    int ret = handle_frame(&header, payload);

    plaintext_buffer_->skip(header.length);

    return ret;
}

int Http2Handler::handle_frame(const Http2FrameHeader* header, const uint8_t* payload) {
    switch (header->type) {
        case Http2FrameType::SETTINGS:
            return handle_settings_frame(payload, header->length);
        case Http2FrameType::HEADERS:
            return handle_headers_frame(header->stream_id, payload, header->length, header->flags);
        case Http2FrameType::DATA:
            return handle_data_frame(header->stream_id, payload, header->length, header->flags);
        case Http2FrameType::CONTINUATION:
            return handle_continuation_frame(header->stream_id, payload, header->length, header->flags);
        case Http2FrameType::WINDOW_UPDATE:
            if (header->length >= 4) {
                uint32_t increment = (static_cast<uint32_t>(payload[0]) << 24) |
                                    (static_cast<uint32_t>(payload[1]) << 16) |
                                    (static_cast<uint32_t>(payload[2]) << 8) |
                                    static_cast<uint32_t>(payload[3]);
                increment &= 0x7FFFFFFF;
                return handle_window_update_frame(header->stream_id, increment);
            }
            break;
        case Http2FrameType::RST_STREAM:
            if (header->length >= 4) {
                uint32_t error_code = (static_cast<uint32_t>(payload[0]) << 24) |
                                     (static_cast<uint32_t>(payload[1]) << 16) |
                                     (static_cast<uint32_t>(payload[2]) << 8) |
                                     static_cast<uint32_t>(payload[3]);
                return handle_rst_stream_frame(header->stream_id, error_code);
            }
            break;
        case Http2FrameType::PING:
            return handle_ping_frame(payload, header->length);
        case Http2FrameType::GOAWAY:
            return handle_goaway_frame(payload, header->length);
        default:
            break;
    }
    return PROTOCOL_OK;
}

int Http2Handler::handle_settings_frame(const uint8_t* payload, size_t len) {
    if (len == 0) {
        settings_ack_received_ = true;
        return PROTOCOL_OK;
    }

    size_t pos = 0;
    while (pos + 6 <= len) {
        uint16_t id = (static_cast<uint16_t>(payload[pos]) << 8) |
                     static_cast<uint16_t>(payload[pos + 1]);
        uint32_t value = (static_cast<uint32_t>(payload[pos + 2]) << 24) |
                        (static_cast<uint32_t>(payload[pos + 3]) << 16) |
                        (static_cast<uint32_t>(payload[pos + 4]) << 8) |
                        static_cast<uint32_t>(payload[pos + 5]);
        pos += 6;

        switch (id) {
            case 1: settings_.header_table_size = value; break;
            case 2: settings_.enable_push = value; break;
            case 3: settings_.max_concurrent_streams = value; break;
            case 4: settings_.initial_window_size = value; break;
            case 5: settings_.max_frame_size = value; break;
            case 6: settings_.max_header_list_size = value; break;
            default: break;
        }
    }

    return send_settings_ack();
}

int Http2Handler::handle_headers_frame(uint32_t stream_id, const uint8_t* payload,
                                        size_t len, uint8_t flags) {
    Http2Stream* stream = get_or_create_stream(stream_id);
    if (!stream) {
        return PROTOCOL_ERROR_INVALID;
    }

    if (stream->state == Http2StreamState::IDLE) {
        stream->state = Http2StreamState::OPEN;
    }

    std::map<std::string, std::string> headers;
    int ret = hpack_decoder_->decode(payload, len, &headers);
    if (ret != PROTOCOL_OK) {
        return ret;
    }

    for (const auto& pair : headers) {
        stream->request.headers[pair.first] = pair.second;
    }

    if (!(flags & HTTP2_FLAG_END_HEADERS)) {
        return PROTOCOL_OK;
    }

    if (flags & HTTP2_FLAG_END_STREAM) {
        stream->state = Http2StreamState::HALF_CLOSED_REMOTE;
        return handle_complete_request(stream);
    }

    return PROTOCOL_OK;
}

int Http2Handler::handle_data_frame(uint32_t stream_id, const uint8_t* payload,
                                     size_t len, uint8_t flags) {
    auto it = streams_.find(stream_id);
    if (it == streams_.end()) {
        return PROTOCOL_ERROR_INVALID;
    }

    Http2Stream* stream = it->second.get();
    if (len > 0 && payload != nullptr) {
        stream->request.body.insert(stream->request.body.end(), payload, payload + len);
    }

    if (flags & HTTP2_FLAG_END_STREAM) {
        stream->state = Http2StreamState::HALF_CLOSED_REMOTE;
        return handle_complete_request(stream);
    }

    return PROTOCOL_OK;
}

int Http2Handler::handle_continuation_frame(uint32_t stream_id, const uint8_t* payload,
                                             size_t len, uint8_t flags) {
    auto it = streams_.find(stream_id);
    if (it == streams_.end()) {
        return PROTOCOL_ERROR_INVALID;
    }

    Http2Stream* stream = it->second.get();

    std::map<std::string, std::string> headers;
    int ret = hpack_decoder_->decode(payload, len, &headers);
    if (ret != PROTOCOL_OK) {
        return ret;
    }

    for (const auto& pair : headers) {
        stream->request.headers[pair.first] = pair.second;
    }

    if (flags & HTTP2_FLAG_END_HEADERS) {
        if (stream->state == Http2StreamState::HALF_CLOSED_REMOTE) {
            return handle_complete_request(stream);
        }
    }

    return PROTOCOL_OK;
}

int Http2Handler::handle_window_update_frame(uint32_t stream_id, uint32_t increment) {
    // 检查整数溢出
    constexpr uint32_t MAX_WINDOW = 0x7FFFFFFF;
    if (increment == 0 || increment > MAX_WINDOW) {
        return PROTOCOL_ERROR_INVALID;
    }

    int32_t incr = static_cast<int32_t>(increment);
    int32_t max_win = static_cast<int32_t>(MAX_WINDOW);

    if (stream_id == 0) {
        for (auto& pair : streams_) {
            if (pair.second->send_window > max_win - incr) {
                pair.second->send_window = max_win;
            } else {
                pair.second->send_window += incr;
            }
        }
    } else {
        auto it = streams_.find(stream_id);
        if (it != streams_.end()) {
            if (it->second->send_window > max_win - incr) {
                it->second->send_window = max_win;
            } else {
                it->second->send_window += incr;
            }
        }
    }
    return PROTOCOL_OK;
}

int Http2Handler::handle_rst_stream_frame(uint32_t stream_id, uint32_t error_code) {
    (void)error_code;
    auto it = streams_.find(stream_id);
    if (it != streams_.end()) {
        it->second->state = Http2StreamState::CLOSED;
    }
    return PROTOCOL_OK;
}

int Http2Handler::handle_ping_frame(const uint8_t* payload, size_t len) {
    // 发送PING ACK响应
    if (len >= 8) {
        write_frame(Http2FrameType::PING, 0, HTTP2_FLAG_ACK, payload, 8);
    }
    return PROTOCOL_OK;
}

int Http2Handler::handle_goaway_frame(const uint8_t* payload, size_t len) {
    (void)payload;
    (void)len;
    // 停止接受新stream，这里我们标记一下
    // 在实际实现中应该拒绝新的stream
    return PROTOCOL_OK;
}

int Http2Handler::send_settings_frame() {
    // 使用辅助函数写入 SETTINGS 参数
    struct SettingParam {
        uint16_t id;
        uint32_t value;
    };

    SettingParam params[] = {
        {1, local_settings_.header_table_size},
        {2, local_settings_.enable_push},
        {3, local_settings_.max_concurrent_streams},
        {4, local_settings_.initial_window_size},
        {5, local_settings_.max_frame_size}
    };

    // 每个参数6字节 (2字节ID + 4字节值)，共5个参数
    uint8_t payload[30];  // 5 * 6 = 30 字节
    size_t pos = 0;

    for (const auto& param : params) {
        // 写入参数ID (2字节)
        payload[pos++] = static_cast<uint8_t>((param.id >> 8) & 0xFF);
        payload[pos++] = static_cast<uint8_t>(param.id & 0xFF);
        // 写入参数值 (4字节)
        payload[pos++] = static_cast<uint8_t>((param.value >> 24) & 0xFF);
        payload[pos++] = static_cast<uint8_t>((param.value >> 16) & 0xFF);
        payload[pos++] = static_cast<uint8_t>((param.value >> 8) & 0xFF);
        payload[pos++] = static_cast<uint8_t>(param.value & 0xFF);
    }

    return write_frame(Http2FrameType::SETTINGS, 0, 0, payload, pos);
}

int Http2Handler::send_settings_ack() {
    if (!settings_ack_sent_) {
        settings_ack_sent_ = true;
        return write_frame(Http2FrameType::SETTINGS, 0, HTTP2_FLAG_ACK, nullptr, 0);
    }
    return PROTOCOL_OK;
}

int Http2Handler::send_headers_frame(uint32_t stream_id,
                                      const std::map<std::string, std::string>& headers,
                                      bool end_stream) {
    std::vector<uint8_t> payload;
    int ret = hpack_encoder_->encode(headers, &payload);
    if (ret != PROTOCOL_OK) {
        return ret;
    }

    uint8_t flags = HTTP2_FLAG_END_HEADERS;
    if (end_stream) {
        flags |= HTTP2_FLAG_END_STREAM;
    }

    // 检查是否需要分割为CONTINUATION帧
    size_t max_payload = local_settings_.max_frame_size;
    if (payload.size() <= max_payload) {
        return write_frame(Http2FrameType::HEADERS, stream_id, flags, payload.data(), payload.size());
    }

    // 需要分割: HEADERS + CONTINUATION帧
    size_t offset = 0;

    // 第一个HEADERS帧（不带END_HEADERS）
    uint8_t first_flags = flags & ~HTTP2_FLAG_END_HEADERS;
    ret = write_frame(Http2FrameType::HEADERS, stream_id, first_flags,
                      payload.data(), max_payload);
    if (ret != PROTOCOL_OK) {
        return ret;
    }
    offset += max_payload;

    // 中间的CONTINUATION帧（不带END_HEADERS）
    while (offset + max_payload < payload.size()) {
        ret = write_frame(Http2FrameType::CONTINUATION, stream_id, 0,
                          payload.data() + offset, max_payload);
        if (ret != PROTOCOL_OK) {
            return ret;
        }
        offset += max_payload;
    }

    // 最后的CONTINUATION帧（带END_HEADERS）
    uint8_t last_flags = HTTP2_FLAG_END_HEADERS;
    return write_frame(Http2FrameType::CONTINUATION, stream_id, last_flags,
                      payload.data() + offset, payload.size() - offset);
}

int Http2Handler::send_data_frame(uint32_t stream_id, const uint8_t* data,
                                   size_t len, bool end_stream) {
    size_t max_size = local_settings_.max_frame_size;
    size_t offset = 0;

    while (offset < len) {
        size_t chunk_size = std::min(len - offset, max_size);
        bool last_chunk = (offset + chunk_size == len);
        uint8_t flags = (last_chunk && end_stream) ? HTTP2_FLAG_END_STREAM : 0;

        int ret = write_frame(Http2FrameType::DATA, stream_id, flags,
                              data + offset, chunk_size);
        if (ret != PROTOCOL_OK) {
            return ret;
        }

        offset += chunk_size;
    }

    return PROTOCOL_OK;
}

int Http2Handler::send_window_update_frame(uint32_t stream_id, uint32_t increment) {
    uint8_t payload[4];
    payload[0] = (increment >> 24) & 0x7F;
    payload[1] = (increment >> 16) & 0xFF;
    payload[2] = (increment >> 8) & 0xFF;
    payload[3] = increment & 0xFF;
    return write_frame(Http2FrameType::WINDOW_UPDATE, stream_id, 0, payload, 4);
}

int Http2Handler::send_rst_stream_frame(uint32_t stream_id, uint32_t error_code) {
    uint8_t payload[4];
    payload[0] = (error_code >> 24) & 0xFF;
    payload[1] = (error_code >> 16) & 0xFF;
    payload[2] = (error_code >> 8) & 0xFF;
    payload[3] = error_code & 0xFF;
    return write_frame(Http2FrameType::RST_STREAM, stream_id, 0, payload, 4);
}

int Http2Handler::write_frame(Http2FrameType type, uint32_t stream_id,
                               uint8_t flags, const uint8_t* payload, size_t len) {
    uint8_t header[HTTP2_FRAME_HEADER_SIZE];

    header[0] = (len >> 16) & 0xFF;
    header[1] = (len >> 8) & 0xFF;
    header[2] = len & 0xFF;
    header[3] = static_cast<uint8_t>(type);
    header[4] = flags;
    header[5] = (stream_id >> 24) & 0x7F;
    header[6] = (stream_id >> 16) & 0xFF;
    header[7] = (stream_id >> 8) & 0xFF;
    header[8] = stream_id & 0xFF;

    // 只通过TlsHandler进行数据加密发送，避免双重写入
    if (tls_handler_) {
        size_t written = 0;
        int ret = tls_handler_->write(header, HTTP2_FRAME_HEADER_SIZE, &written);
        if (ret != PROTOCOL_OK) {
            return ret;
        }
        if (written != HTTP2_FRAME_HEADER_SIZE) {
            return PROTOCOL_ERROR_BUFFER;
        }
        if (len > 0 && payload != nullptr) {
            ret = tls_handler_->write(payload, len, &written);
            if (ret != PROTOCOL_OK) {
                return ret;
            }
            if (written != len) {
                return PROTOCOL_ERROR_BUFFER;
            }
        }
    }

    return PROTOCOL_OK;
}

Http2Stream* Http2Handler::get_or_create_stream(uint32_t stream_id) {
    auto it = streams_.find(stream_id);
    if (it != streams_.end()) {
        return it->second.get();
    }

    auto stream = std::make_unique<Http2Stream>();
    stream->init(stream_id);
    Http2Stream* ptr = stream.get();
    streams_[stream_id] = std::move(stream);
    return ptr;
}

int Http2Handler::handle_complete_request(Http2Stream* stream) {
    // 空指针检查
    if (conn_ == nullptr || stream == nullptr) {
        return PROTOCOL_ERROR_INVALID;
    }

    // 构建ClientContext并调用Callback模块
    // 注意：将字符串复制到局部变量，确保指针在函数执行期间有效（修复悬空指针问题）
    ClientContext ctx;
    const ClientInfo& client_info = conn_->get_client_info();

    std::string local_client_ip = client_info.client_ip;
    std::string local_debug_token;
    bool found_token = false;

    ctx.connection_id = client_info.connection_id;
    ctx.client_ip = local_client_ip.c_str();
    ctx.client_port = client_info.client_port;
    ctx.server_port = client_info.server_port;

    // 从header中获取debug token（大小写不敏感查找）
    for (const auto& header : stream->request.headers) {
        if (StrCaseCmp(header.first.c_str(), "Debug-Token") == 0) {
            local_debug_token = header.second;
            found_token = true;
            break;
        }
    }
    if (found_token) {
        ctx.token = local_debug_token.c_str();
    } else {
        ctx.token = nullptr;
    }

    // 模拟Callback模块调用
    std::map<std::string, std::string> response_headers;
    response_headers[":status"] = "200";
    response_headers["content-type"] = "text/plain";

    send_headers_frame(stream->stream_id, response_headers, false);

    // 如果有请求体，回显请求体，否则返回"OK"
    if (!stream->request.body.empty()) {
        send_data_frame(stream->stream_id, stream->request.body.data(),
                        stream->request.body.size(), true);
    } else {
        const char* reply = "OK";
        send_data_frame(stream->stream_id, reinterpret_cast<const uint8_t*>(reply), 2, true);
    }

    return PROTOCOL_OK;
}

// ==================== ProtocolFactory实现 ====================

ProtocolFactory& ProtocolFactory::instance() {
    static ProtocolFactory factory;
    return factory;
}

ProtocolFactory::ProtocolFactory() = default;
ProtocolFactory::~ProtocolFactory() = default;

ProtocolHandler* ProtocolFactory::create_handler(ProtocolType type) {
    switch (type) {
        case ProtocolType::HTTP_1_1:
            return new Http1Handler();
        case ProtocolType::HTTP_2:
            return new Http2Handler();
        default:
            return nullptr;
    }
}

ProtocolHandler* ProtocolFactory::create_handler(ProtocolType type, Connection* conn) {
    // 关联用例: PROTO-001 修复
    // 注意: 当前Http1Handler/Http2Handler不在构造函数中接收Connection
    // Connection通过init()方法传入，这里我们创建处理器后返回，调用方需调用init()
    (void)conn;  // 为了兼容性保留参数
    return create_handler(type);
}

void ProtocolFactory::destroy_handler(ProtocolHandler* handler) {
    delete handler;
}

ProtocolType ProtocolFactory::select_protocol_by_alpn(const std::string& alpn) {
    if (alpn == "h2") {
        return ProtocolType::HTTP_2;
    } else if (alpn == "http/1.1") {
        return ProtocolType::HTTP_1_1;
    }
    return ProtocolType::HTTP_1_1;
}

} // namespace protocol
} // namespace https_server_sim

// 文件结束
