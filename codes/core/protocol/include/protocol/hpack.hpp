// =============================================================================
//  HTTPS Server Simulator - Protocol Module
//  文件: hpack.hpp
//  描述: HpackEncoder和HpackDecoder类定义（简化版HPACK头部压缩实现）
//  版权: Copyright (c) 2026
// =============================================================================
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
