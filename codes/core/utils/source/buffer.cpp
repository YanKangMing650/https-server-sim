#include "utils/buffer.hpp"
#include <cstring>
#include <algorithm>

namespace https_server_sim {
namespace utils {

Buffer::Buffer(size_t initial_capacity)
    : read_idx_(0), write_idx_(0)
{
    size_t cap = std::max(initial_capacity, MIN_CAPACITY);
    cap = std::min(cap, MAX_CAPACITY);
    data_.resize(cap);
}

Buffer::~Buffer() = default;

Buffer::Buffer(Buffer&& other) noexcept
    : data_(std::move(other.data_))
    , read_idx_(other.read_idx_)
    , write_idx_(other.write_idx_)
{
    other.read_idx_ = 0;
    other.write_idx_ = 0;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        data_ = std::move(other.data_);
        read_idx_ = other.read_idx_;
        write_idx_ = other.write_idx_;
        other.read_idx_ = 0;
        other.write_idx_ = 0;
    }
    return *this;
}

size_t Buffer::write(const uint8_t* data, size_t len) {
    if (len == 0 || data == nullptr) {
        return 0;
    }
    if (!ensure_writable(len)) {
        return 0;
    }
    std::memcpy(write_ptr(), data, len);
    commit(len);
    return len;
}

size_t Buffer::write(const char* data, size_t len) {
    return write(reinterpret_cast<const uint8_t*>(data), len);
}

size_t Buffer::write_uint8(uint8_t value) {
    return write(&value, 1);
}

size_t Buffer::write_char(char value) {
    return write(reinterpret_cast<uint8_t*>(&value), 1);
}

size_t Buffer::write_uint16_be(uint16_t value) {
    uint8_t buf[2];
    buf[0] = static_cast<uint8_t>((value >> 8) & 0xFF);
    buf[1] = static_cast<uint8_t>(value & 0xFF);
    return write(buf, 2);
}

size_t Buffer::write_uint32_be(uint32_t value) {
    uint8_t buf[4];
    buf[0] = static_cast<uint8_t>((value >> 24) & 0xFF);
    buf[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
    buf[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
    buf[3] = static_cast<uint8_t>(value & 0xFF);
    return write(buf, 4);
}

size_t Buffer::write_uint64_be(uint64_t value) {
    uint8_t buf[8];
    buf[0] = static_cast<uint8_t>((value >> 56) & 0xFF);
    buf[1] = static_cast<uint8_t>((value >> 48) & 0xFF);
    buf[2] = static_cast<uint8_t>((value >> 40) & 0xFF);
    buf[3] = static_cast<uint8_t>((value >> 32) & 0xFF);
    buf[4] = static_cast<uint8_t>((value >> 24) & 0xFF);
    buf[5] = static_cast<uint8_t>((value >> 16) & 0xFF);
    buf[6] = static_cast<uint8_t>((value >> 8) & 0xFF);
    buf[7] = static_cast<uint8_t>(value & 0xFF);
    return write(buf, 8);
}

uint8_t* Buffer::reserve(size_t len) {
    if (!ensure_writable(len)) {
        return nullptr;
    }
    return write_ptr();
}

void Buffer::commit(size_t len) {
    // 安全边界检查：只提交不超过当前可写空间的数据量
    // 这样即使传入过大的len也不会导致write_idx_越界
    write_idx_ += std::min(len, writable_bytes());
}

size_t Buffer::read(uint8_t* data, size_t len) {
    size_t available = readable_bytes();
    size_t to_read = std::min(len, available);
    if (to_read > 0 && data != nullptr) {
        std::memcpy(data, read_ptr(), to_read);
    }
    skip(to_read);
    return to_read;
}

size_t Buffer::read(char* data, size_t len) {
    return read(reinterpret_cast<uint8_t*>(data), len);
}

uint8_t Buffer::read_uint8() {
    uint8_t value = 0;
    read(&value, 1);
    return value;
}

char Buffer::read_char() {
    char value = 0;
    read(&value, 1);
    return value;
}

uint16_t Buffer::read_uint16_be() {
    uint8_t buf[2] = {0};
    read(buf, 2);
    return (static_cast<uint16_t>(buf[0]) << 8) |
           static_cast<uint16_t>(buf[1]);
}

uint32_t Buffer::read_uint32_be() {
    uint8_t buf[4] = {0};
    read(buf, 4);
    return (static_cast<uint32_t>(buf[0]) << 24) |
           (static_cast<uint32_t>(buf[1]) << 16) |
           (static_cast<uint32_t>(buf[2]) << 8) |
           static_cast<uint32_t>(buf[3]);
}

uint64_t Buffer::read_uint64_be() {
    uint8_t buf[8] = {0};
    read(buf, 8);
    return (static_cast<uint64_t>(buf[0]) << 56) |
           (static_cast<uint64_t>(buf[1]) << 48) |
           (static_cast<uint64_t>(buf[2]) << 40) |
           (static_cast<uint64_t>(buf[3]) << 32) |
           (static_cast<uint64_t>(buf[4]) << 24) |
           (static_cast<uint64_t>(buf[5]) << 16) |
           (static_cast<uint64_t>(buf[6]) << 8) |
           static_cast<uint64_t>(buf[7]);
}

size_t Buffer::peek(uint8_t* data, size_t len) const {
    size_t available = readable_bytes();
    size_t to_read = std::min(len, available);
    if (to_read > 0 && data != nullptr) {
        std::memcpy(data, read_ptr(), to_read);
    }
    return to_read;
}

size_t Buffer::peek(char* data, size_t len) const {
    return peek(reinterpret_cast<uint8_t*>(data), len);
}

uint8_t Buffer::peek_uint8(size_t offset) const {
    if (offset >= readable_bytes()) {
        return 0;
    }
    return data_[read_idx_ + offset];
}

const uint8_t* Buffer::read_ptr() const {
    return data_.data() + read_idx_;
}

uint8_t* Buffer::write_ptr() {
    return data_.data() + write_idx_;
}

void Buffer::skip(size_t len) {
    read_idx_ += std::min(len, readable_bytes());
}

void Buffer::unskip(size_t len) {
    if (len <= read_idx_) {
        read_idx_ -= len;
    } else {
        read_idx_ = 0;
    }
}

size_t Buffer::readable_bytes() const {
    return write_idx_ - read_idx_;
}

size_t Buffer::writable_bytes() const {
    return data_.size() - write_idx_;
}

size_t Buffer::capacity() const {
    return data_.size();
}

bool Buffer::ensure_writable(size_t len) {
    if (writable_bytes() >= len) {
        return true;
    }
    // 先尝试压缩
    if (read_idx_ > 0) {
        compact();
        if (writable_bytes() >= len) {
            return true;
        }
    }
    // 需要扩容
    size_t required = write_idx_ + len;
    size_t new_capacity = calculate_growth(required);
    return resize(new_capacity);
}

bool Buffer::reserve_capacity(size_t new_capacity) {
    if (new_capacity <= capacity()) {
        return true;
    }
    if (new_capacity > MAX_CAPACITY) {
        return false;
    }
    return resize(new_capacity);
}

void Buffer::shrink_to_fit() {
    size_t needed = readable_bytes();
    if (needed == 0) {
        reset();
        return;
    }
    compact();
    if (write_idx_ < data_.size()) {
        data_.resize(write_idx_);
    }
}

void Buffer::clear() {
    read_idx_ = 0;
    write_idx_ = 0;
}

void Buffer::compact() {
    if (read_idx_ == 0) {
        return;
    }
    size_t data_len = write_idx_ - read_idx_;
    if (data_len > 0) {
        std::memmove(data_.data(), data_.data() + read_idx_, data_len);
    }
    read_idx_ = 0;
    write_idx_ = data_len;
}

void Buffer::reset() {
    read_idx_ = 0;
    write_idx_ = 0;
    data_.resize(DEFAULT_INITIAL_CAPACITY);
}

size_t Buffer::calculate_growth(size_t required) const {
    size_t current = data_.size();
    size_t new_capacity = current;

    if (current < GROWTH_THRESHOLD_DOUBLE) {
        // 小于64KB：翻倍
        new_capacity = current * 2;
    } else if (current < GROWTH_THRESHOLD_15X) {
        // 64KB ~ 1MB：1.5倍
        new_capacity = current + (current / 2);
    } else {
        // 大于1MB：线性增长
        new_capacity = current + GROWTH_LINEAR_INCREMENT;
    }

    // 确保满足需求
    new_capacity = std::max(new_capacity, required);
    // 不超过最大值
    new_capacity = std::min(new_capacity, MAX_CAPACITY);

    return new_capacity;
}

bool Buffer::resize(size_t new_capacity) {
    if (new_capacity > MAX_CAPACITY) {
        return false;
    }
    try {
        data_.resize(new_capacity);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace utils
} // namespace https_server_sim
