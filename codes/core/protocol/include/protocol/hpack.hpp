// =============================================================================
//  HTTPS Server Simulator - Protocol Module
//  文件: hpack.hpp
//  描述: HpackEncoder和HpackDecoder类定义
//  版权: Copyright (c) 2026
//
//  ================================== WARNING ==================================
//  PROTO2-001（严重级别问题）:
//  当前实现是简化版HPACK，未按设计文档要求使用nghttp2库。
//
//  设计文档明确要求：
//  - 使用nghttp2库实现完整HPACK（含动态表管理）
//  - 使用nghttp2_hd_deflate_*系列函数进行编码
//  - 使用nghttp2_hd_inflate_*系列函数进行解码
//
//  TODO: 未来重构需要做以下改动：
//  1. 包含nghttp2/nghttp2.h头文件
//  2. HpackEncoder类添加void* nghttp2_encoder_成员变量
//  3. HpackDecoder类添加void* nghttp2_decoder_成员变量
//  4. 构造函数中调用nghttp2_hd_deflate_new()/nghttp2_hd_inflate_new()
//  5. 析构函数中调用nghttp2_hd_deflate_del()/nghttp2_hd_inflate_del()
//  6. encode()方法使用nghttp2_hd_deflate_deflate()
//  7. decode()方法使用nghttp2_hd_inflate_hd()
//  8. 正确管理动态表大小和状态
//
//  当前简化实现的限制：
//  - 仅支持静态表索引，不支持动态表
//  - 不支持Huffman编码
//  - 不支持动态表大小更新的完整语义
//  =============================================================================
#pragma once

#include "protocol/protocol_types.hpp"
#include <cstdint>
#include <vector>
#include <map>
#include <string>

namespace https_server_sim {
namespace protocol {

// ==================== HPACK编码器类 ====================
// TODO(PROTO2-001): 应使用nghttp2库实现，当前为简化版本
class HpackEncoder {
public:
    /**
     * @brief 构造函数
     * TODO(PROTO2-001): 应初始化nghttp2_hd_deflate编码器上下文
     */
    HpackEncoder();

    /**
     * @brief 析构函数
     * TODO(PROTO2-001): 应释放nghttp2_hd_deflate编码器上下文
     */
    ~HpackEncoder();

    // 禁止拷贝
    HpackEncoder(const HpackEncoder&) = delete;
    HpackEncoder& operator=(const HpackEncoder&) = delete;

    /**
     * @brief 编码HTTP头部
     * @param headers 头部集合
     * @param out 输出编码后的数据
     * @return 0成功，负数失败
     * TODO(PROTO2-001): 应使用nghttp2_hd_deflate_deflate()实现
     */
    int encode(const std::map<std::string, std::string>& headers,
               std::vector<uint8_t>* out);

    /**
     * @brief 设置动态表最大大小
     * @param size 大小
     * TODO(PROTO2-001): 应调用nghttp2_hd_deflate_change_table_size()
     */
    void set_max_dynamic_table_size(uint32_t size);

    /**
     * @brief 获取当前动态表最大大小
     * @return 大小
     */
    uint32_t get_max_dynamic_table_size() const;

private:
    // TODO(PROTO2-001): 应添加 void* nghttp2_encoder_ = nullptr;
    uint32_t max_dynamic_table_size_;
};

// ==================== HPACK解码器类 ====================
// TODO(PROTO2-001): 应使用nghttp2库实现，当前为简化版本
class HpackDecoder {
public:
    /**
     * @brief 构造函数
     * TODO(PROTO2-001): 应初始化nghttp2_hd_inflate解码器上下文
     */
    HpackDecoder();

    /**
     * @brief 析构函数
     * TODO(PROTO2-001): 应释放nghttp2_hd_inflate解码器上下文
     */
    ~HpackDecoder();

    // 禁止拷贝
    HpackDecoder(const HpackDecoder&) = delete;
    HpackDecoder& operator=(const HpackDecoder&) = delete;

    /**
     * @brief 解码HPACK数据
     * @param data 输入数据
     * @param len 数据长度
     * @param headers 输出头部集合
     * @return 0成功，负数失败
     * TODO(PROTO2-001): 应使用nghttp2_hd_inflate_hd()实现
     */
    int decode(const uint8_t* data, size_t len,
               std::map<std::string, std::string>* headers);

    /**
     * @brief 设置动态表最大大小
     * @param size 大小
     * TODO(PROTO2-001): 应调用nghttp2_hd_inflate_change_table_size()
     */
    void set_max_dynamic_table_size(uint32_t size);

    /**
     * @brief 获取当前动态表最大大小
     * @return 大小
     */
    uint32_t get_max_dynamic_table_size() const;

private:
    // TODO(PROTO2-001): 应添加 void* nghttp2_decoder_ = nullptr;
    uint32_t max_dynamic_table_size_;
};

// ==================== 简化的HPACK编码/解码函数 ====================

/**
 * @brief HPACK编码（无需创建对象）
 * @param headers 头部集合
 * @param out 输出编码后的数据
 * @return 0成功，负数失败
 * TODO(PROTO2-001): 内部应使用nghttp2库实现
 */
int hpack_encode(const std::map<std::string, std::string>& headers,
                 std::vector<uint8_t>* out);

/**
 * @brief HPACK解码（无需创建对象）
 * @param data 输入数据
 * @param len 数据长度
 * @param headers 输出头部集合
 * @return 0成功，负数失败
 * TODO(PROTO2-001): 内部应使用nghttp2库实现
 */
int hpack_decode(const uint8_t* data, size_t len,
                 std::map<std::string, std::string>* headers);

} // namespace protocol
} // namespace https_server_sim

// 文件结束
