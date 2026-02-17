// Utils模块单元测试

#include <gtest/gtest.h>
#include "utils/error.hpp"
#include "utils/time.hpp"
#include "utils/buffer.hpp"

using namespace https_server_sim::utils;

TEST(UtilsTest, ErrorCodeToString) {
    EXPECT_STREQ(error_code_to_string(ErrorCode::SUCCESS), "SUCCESS");
    EXPECT_STREQ(error_code_to_string(ErrorCode::UNKNOWN_ERROR), "UNKNOWN_ERROR");
}

TEST(UtilsTest, IsSuccess) {
    EXPECT_TRUE(is_success(ErrorCode::SUCCESS));
    EXPECT_FALSE(is_success(ErrorCode::UNKNOWN_ERROR));
}

TEST(UtilsTest, GetCurrentTimeMs) {
    uint64_t time1 = get_current_time_ms();
    EXPECT_GT(time1, 0);
    uint64_t time2 = get_current_time_ms();
    EXPECT_GE(time2, time1);
}

TEST(UtilsTest, StopWatch) {
    StopWatch sw;
    sw.reset();
    uint64_t elapsed = sw.elapsed_ms();
    EXPECT_GE(elapsed, 0);
}

TEST(UtilsTest, BufferCreate) {
    Buffer buf;
    EXPECT_EQ(buf.readable_bytes(), 0);
    EXPECT_GT(buf.capacity(), 0);
}

TEST(UtilsTest, BufferWriteRead) {
    Buffer buf;
    const char* test_data = "Hello, World!";
    size_t len = strlen(test_data);

    size_t written = buf.write(reinterpret_cast<const uint8_t*>(test_data), len);
    EXPECT_EQ(written, len);
    EXPECT_EQ(buf.readable_bytes(), len);

    char read_buf[32] = {0};
    size_t read = buf.read(read_buf, len);
    EXPECT_EQ(read, len);
    EXPECT_STREQ(read_buf, test_data);
}

TEST(UtilsTest, BufferClear) {
    Buffer buf;
    buf.write_uint8(0xAB);
    EXPECT_EQ(buf.readable_bytes(), 1);
    buf.clear();
    EXPECT_EQ(buf.readable_bytes(), 0);
}

TEST(UtilsTest, BufferPeek) {
    Buffer buf;
    buf.write_uint8(0x12);
    buf.write_uint8(0x34);

    EXPECT_EQ(buf.peek_uint8(0), 0x12);
    EXPECT_EQ(buf.peek_uint8(1), 0x34);
    EXPECT_EQ(buf.readable_bytes(), 2);
}

TEST(UtilsTest, BufferNetworkOrder) {
    Buffer buf;
    uint16_t val16 = 0x1234;
    uint32_t val32 = 0x12345678;
    uint64_t val64 = 0x123456789ABCDEF0;

    buf.write_uint16_be(val16);
    buf.write_uint32_be(val32);
    buf.write_uint64_be(val64);

    EXPECT_EQ(buf.read_uint16_be(), val16);
    EXPECT_EQ(buf.read_uint32_be(), val32);
    EXPECT_EQ(buf.read_uint64_be(), val64);
}

TEST(UtilsTest, BufferMove) {
    Buffer buf1;
    buf1.write_uint8(0xAA);

    Buffer buf2(std::move(buf1));
    EXPECT_EQ(buf2.readable_bytes(), 1);
    EXPECT_EQ(buf2.read_uint8(), 0xAA);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
