// =============================================================================
//  HTTPS Server Simulator - Utils Module
//  文件: buffer.hpp
//  描述: 动态缓冲区类定义
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace https_server_sim {
namespace utils {

/**
 * @brief 动态缓冲区类
 * @note 线程安全说明：Buffer 类不是线程安全的。如果需要在多线程环境中使用，
 *       调用方必须负责使用外部同步机制（如 mutex）来保护对 Buffer 的访问。
 */
class Buffer {
public:
    static constexpr size_t DEFAULT_INITIAL_CAPACITY = 8192;  // 默认初始容量
    static constexpr size_t MIN_CAPACITY = 1024;              // 最小容量
    static constexpr size_t MAX_CAPACITY = 64 * 1024 * 1024;  // 最大容量
    static constexpr size_t GROWTH_THRESHOLD_DOUBLE = 64 * 1024;  // 翻倍扩容阈值
    static constexpr size_t GROWTH_THRESHOLD_15X = 1024 * 1024;   // 1.5倍扩容阈值
    static constexpr size_t GROWTH_LINEAR_INCREMENT = 256 * 1024;  // 线性扩容增量

    // 构造函数
    // initial_capacity: 初始容量，默认8KB
    explicit Buffer(size_t initial_capacity = DEFAULT_INITIAL_CAPACITY);

    // 析构函数
    ~Buffer();

    // 禁止拷贝
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    // 支持移动
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    // ========== 写入方法 ==========

    // 写入数据，返回实际写入的字节数
    // 会自动扩容，超过MAX_CAPACITY时返回0
    size_t write(const uint8_t* data, size_t len);
    size_t write(const char* data, size_t len);

    // 写入std::string（便捷方法）
    size_t write(const std::string& str) {
        return write(str.data(), str.size());
    }

    // 写入单个字节
    size_t write_uint8(uint8_t value);
    size_t write_char(char value);

    // 写入16位/32位/64位整数（网络字节序）
    size_t write_uint16_be(uint16_t value);
    size_t write_uint32_be(uint32_t value);
    size_t write_uint64_be(uint64_t value);

    // 预留可写空间，返回指向该空间的指针
    // 用于直接写入（如recv/send）
    uint8_t* reserve(size_t len);

    // 提交已写入的数据（配合reserve使用）
    void commit(size_t len);

    // ========== 读取方法 ==========

    // 读取数据，返回实际读取的字节数，移动读指针
    size_t read(uint8_t* data, size_t len);
    size_t read(char* data, size_t len);

    // 读取数据到std::string（便捷方法）
    // len: 要读取的字节数，0表示读取所有可读数据
    size_t read_to_string(std::string* out, size_t len = 0) {
        if (out == nullptr) {
            return 0;
        }
        size_t to_read = (len == 0) ? readable_bytes() : std::min(len, readable_bytes());
        if (to_read == 0) {
            out->clear();
            return 0;
        }
        out->resize(to_read);
        return read(reinterpret_cast<uint8_t*>(&(*out)[0]), to_read);
    }

    // 读取单个字节，移动读指针
    // 注意：数据不足时静默返回0，建议使用带success参数的版本
    uint8_t read_uint8();
    char read_char();

    // 读取单个字节（安全版本），移动读指针
    // success: [out] 是否成功读取
    // return: 读取的数据（仅当success为true时有效）
    uint8_t read_uint8(bool& success);
    char read_char(bool& success);

    // 读取16位/32位/64位整数（网络字节序），移动读指针
    // 注意：数据不足时静默返回0，建议使用带success参数的版本
    uint16_t read_uint16_be();
    uint32_t read_uint32_be();
    uint64_t read_uint64_be();

    // 读取16位/32位/64位整数（网络字节序，安全版本），移动读指针
    // success: [out] 是否成功读取
    // return: 读取的数据（仅当success为true时有效）
    uint16_t read_uint16_be(bool& success);
    uint32_t read_uint32_be(bool& success);
    uint64_t read_uint64_be(bool& success);

    // 查看数据（不移动读指针）
    size_t peek(uint8_t* data, size_t len) const;
    size_t peek(char* data, size_t len) const;

    // 查看指定偏移处的数据
    // 注意：偏移越界时静默返回0，建议使用带success参数的版本
    uint8_t peek_uint8(size_t offset = 0) const;

    // 查看指定偏移处的数据（安全版本）
    // offset: 偏移量
    // success: [out] 是否成功读取
    // return: 读取的数据（仅当success为true时有效）
    uint8_t peek_uint8(size_t offset, bool& success) const;

    // ========== 指针操作 ==========

    // 获取读指针
    const uint8_t* read_ptr() const;

    // 获取写指针
    uint8_t* write_ptr();

    // 跳过数据（移动读指针）
    void skip(size_t len);

    // 回退读指针
    void unskip(size_t len);

    // ========== 容量管理 ==========

    // 获取可读字节数
    size_t readable_bytes() const;

    // 获取可写字节数（不扩容情况下）
    size_t writable_bytes() const;

    // 获取总容量
    size_t capacity() const;

    // 确保有足够的可写空间，必要时扩容
    // return: true-成功，false-失败（超过MAX_CAPACITY）
    bool ensure_writable(size_t len);

    // 预分配容量
    // new_capacity: 新的容量，必须大于当前容量
    // return: true-成功，false-失败
    bool reserve_capacity(size_t new_capacity);

    // 收缩容量到刚好容纳可读数据
    void shrink_to_fit();

    // ========== 清理操作 ==========

    // 清空缓冲区（不释放内存）
    void clear();

    // 压缩空间（将可读数据移到开头，回收空间）
    void compact();

    // 重置缓冲区（释放内存，恢复初始容量）
    void reset();

private:
    // 计算扩容后的容量
    // required: 需要的最小容量
    // return: 新容量，不超过MAX_CAPACITY
    size_t calculate_growth(size_t required) const;

    // 扩容到新容量
    // return: true-成功
    bool resize(size_t new_capacity);

    std::vector<uint8_t> data_;
    size_t read_idx_;
    size_t write_idx_;
};

} // namespace utils
} // namespace https_server_sim
