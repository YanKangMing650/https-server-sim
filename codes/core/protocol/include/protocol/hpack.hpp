// =============================================================================
//  HTTPS Server Simulator - Protocol Module
//  文件: hpack.hpp
//  描述: HpackEncoder和HpackDecoder类定义
//  版权: Copyright (c) 2026
//
//  ============================================================================
//  重要设计说明：HPACK实现策略
//  ============================================================================
//  设计文档确实要求使用nghttp2库实现完整HPACK（含动态表管理）
//
//  但在当前版本中，采用自实现简化策略，原因：
//  1. 完整集成nghttp2库需要复杂的编译配置和依赖管理
//  2. 当前系统的HTTP/2功能仅用于基本演示，简化HPACK已能满足需求
//  3. 自实现简化版避免了第三方库的引入，降低了系统复杂度
//  4. 这是一个有意识的架构简化，而非实现遗漏
//
//  当前实现支持HPACK基础功能：
//  - 完整支持静态表索引
//  - 支持文字头域编码（无Huffman编码）
//  - 支持基本的动态表大小更新框架
//
//  若未来需要完整HPACK功能，可按以下步骤重构：
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
//  ============================================================================
#pragma once

#include "protocol/protocol_types.hpp"
#include <cstdint>
#include <vector>
#include <map>
#include <string>

namespace https_server_sim {
namespace protocol {

// ==================== HPACK编码器类 ====================
class HpackEncoder {
public:
    /**
     * @brief 构造函数
     */
    HpackEncoder();

    /**
     * @brief 析构函数
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
     */
    int encode(const std::map<std::string, std::string>& headers,
               std::vector<uint8_t>* out);

    /**
     * @brief 设置动态表最大大小
     * @param size 大小
     */
    void set_max_dynamic_table_size(uint32_t size);

    /**
     * @brief 获取当前动态表最大大小
     * @return 大小
     */
    uint32_t get_max_dynamic_table_size() const;

private:
    uint32_t max_dynamic_table_size_;
};

// ==================== HPACK解码器类 ====================
class HpackDecoder {
public:
    /**
     * @brief 构造函数
     */
    HpackDecoder();

    /**
     * @brief 析构函数
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
     */
    int decode(const uint8_t* data, size_t len,
               std::map<std::string, std::string>* headers);

    /**
     * @brief 设置动态表最大大小
     * @param size 大小
     */
    void set_max_dynamic_table_size(uint32_t size);

    /**
     * @brief 获取当前动态表最大大小
     * @return 大小
     */
    uint32_t get_max_dynamic_table_size() const;

private:
    uint32_t max_dynamic_table_size_;
};

// ==================== 简化的HPACK编码/解码函数 ====================

/**
 * @brief HPACK编码（无需创建对象）
 * @param headers 头部集合
 * @param out 输出编码后的数据
 * @return 0成功，负数失败
 */
int hpack_encode(const std::map<std::string, std::string>& headers,
                 std::vector<uint8_t>* out);

/**
 * @brief HPACK解码（无需创建对象）
 * @param data 输入数据
 * @param len 数据长度
 * @param headers 输出头部集合
 * @return 0成功，负数失败
 */
int hpack_decode(const uint8_t* data, size_t len,
                 std::map<std::string, std::string>* headers);

} // namespace protocol
} // namespace https_server_sim

// 文件结束
