// =============================================================================
//  HTTPS Server Simulator - Utils Module
//  文件: test_utils.cpp
//  描述: Utils模块完整单元测试
//  版权: Copyright (c) 2026
// =============================================================================

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <cstring>
#include "utils/error.hpp"
#include "utils/time.hpp"
#include "utils/buffer.hpp"
#include "utils/logger.hpp"
#include "utils/lockfree_queue.hpp"
#include "utils/statistics.hpp"
#include "utils/config.hpp"

using namespace https_server_sim::utils;

// =============================================================================
// Error模块测试用例
// =============================================================================

TEST(ErrorTest, ErrorCodeToString) {
    EXPECT_STREQ(error_code_to_string(ErrorCode::SUCCESS), "SUCCESS");
    EXPECT_STREQ(error_code_to_string(ErrorCode::UNKNOWN_ERROR), "UNKNOWN_ERROR");
    EXPECT_STREQ(error_code_to_string(ErrorCode::INVALID_ARGUMENT), "INVALID_ARGUMENT");
    EXPECT_STREQ(error_code_to_string(ErrorCode::FILE_NOT_FOUND), "FILE_NOT_FOUND");
    EXPECT_STREQ(error_code_to_string(ErrorCode::CONFIG_PARSE_ERROR), "CONFIG_PARSE_ERROR");
}

TEST(ErrorTest, ErrorCodeToDescription) {
    EXPECT_STRNE(error_code_to_description(ErrorCode::SUCCESS), nullptr);
    EXPECT_STRNE(error_code_to_description(ErrorCode::UNKNOWN_ERROR), nullptr);
}

TEST(ErrorTest, IsSuccess) {
    EXPECT_TRUE(is_success(ErrorCode::SUCCESS));
    EXPECT_FALSE(is_success(ErrorCode::UNKNOWN_ERROR));
}

TEST(ErrorTest, IsError) {
    EXPECT_FALSE(is_error(ErrorCode::SUCCESS));
    EXPECT_TRUE(is_error(ErrorCode::UNKNOWN_ERROR));
}

TEST(ErrorTest, ResultSuccess) {
    Result<int> r = make_ok(42);
    EXPECT_TRUE(r.is_ok());
    EXPECT_FALSE(r.is_err());
    EXPECT_EQ(r.value(), 42);
    EXPECT_EQ(r.error_code(), ErrorCode::SUCCESS);
}

TEST(ErrorTest, ResultFailure) {
    Result<int> r = make_err<int>(ErrorCode::INVALID_ARGUMENT);
    EXPECT_FALSE(r.is_ok());
    EXPECT_TRUE(r.is_err());
    EXPECT_EQ(r.error_code(), ErrorCode::INVALID_ARGUMENT);
}

TEST(ErrorTest, ResultValueOr) {
    Result<int> r1 = make_ok(42);
    EXPECT_EQ(r1.value_or(100), 42);

    Result<int> r2 = make_err<int>(ErrorCode::INVALID_ARGUMENT);
    EXPECT_EQ(r2.value_or(100), 100);
}

TEST(ErrorTest, ResultErrorMessage) {
    Result<int> r = make_err<int>(ErrorCode::INVALID_ARGUMENT, "test error");
    EXPECT_EQ(r.error_message(), "test error");
}

TEST(ErrorTest, ResultVoidSuccess) {
    Result<void> r = make_ok();
    EXPECT_TRUE(r.is_ok());
    EXPECT_FALSE(r.is_err());
}

TEST(ErrorTest, ResultVoidFailure) {
    Result<void> r = make_err(ErrorCode::INVALID_ARGUMENT);
    EXPECT_FALSE(r.is_ok());
    EXPECT_TRUE(r.is_err());
}

// =============================================================================
// Time模块测试用例
// =============================================================================

TEST(TimeTest, GetCurrentTimeMs) {
    uint64_t time1 = get_current_time_ms();
    EXPECT_GT(time1, 0u);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    uint64_t time2 = get_current_time_ms();
    EXPECT_GE(time2, time1);
}

TEST(TimeTest, GetCurrentTimeUs) {
    uint64_t time1 = get_current_time_us();
    EXPECT_GT(time1, 0u);
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    uint64_t time2 = get_current_time_us();
    EXPECT_GE(time2, time1);
}

TEST(TimeTest, GetMonotonicTimeMs) {
    uint64_t time1 = get_monotonic_time_ms();
    EXPECT_GT(time1, 0u);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    uint64_t time2 = get_monotonic_time_ms();
    EXPECT_GE(time2, time1);
}

TEST(TimeTest, StopWatchElapsed) {
    StopWatch sw;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    uint64_t elapsed_ms = sw.elapsed_ms();
    EXPECT_GE(elapsed_ms, 5u);  // 允许误差
}

TEST(TimeTest, StopWatchReset) {
    StopWatch sw;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    sw.reset();
    uint64_t elapsed_ms = sw.elapsed_ms();
    EXPECT_LT(elapsed_ms, 5u);
}

TEST(TimeTest, TimeoutCheckerNotTimeout) {
    TimeoutChecker tc(1000);
    EXPECT_FALSE(tc.is_timeout());
    EXPECT_GT(tc.remaining_ms(), 0u);
}

TEST(TimeoutCheckerTest, Timeout) {
    TimeoutChecker tc(10);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_TRUE(tc.is_timeout());
    EXPECT_EQ(tc.remaining_ms(), 0u);
}

TEST(TimeoutCheckerTest, Reset) {
    TimeoutChecker tc(10);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_TRUE(tc.is_timeout());
    tc.reset();
    EXPECT_FALSE(tc.is_timeout());
}

TEST(TimeTest, FormatCurrentTime) {
    std::string time_str = format_current_time();
    EXPECT_FALSE(time_str.empty());
}

TEST(TimeTest, FormatTime) {
    uint64_t now = get_current_time_ms();
    std::string time_str = format_time(now);
    EXPECT_FALSE(time_str.empty());
}

// =============================================================================
// Buffer模块测试用例
// =============================================================================

TEST(BufferTest, CreateDefault) {
    Buffer buf;
    EXPECT_EQ(buf.readable_bytes(), 0u);
    EXPECT_EQ(buf.capacity(), Buffer::DEFAULT_INITIAL_CAPACITY);
}

TEST(BufferTest, CreateWithCapacity) {
    Buffer buf(16384);
    EXPECT_EQ(buf.capacity(), 16384u);
}

TEST(BufferTest, CreateMinCapacity) {
    Buffer buf(512);
    EXPECT_EQ(buf.capacity(), Buffer::MIN_CAPACITY);
}

TEST(BufferTest, WriteReadBytes) {
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

TEST(BufferTest, WriteReadUint8) {
    Buffer buf;
    buf.write_uint8(0xAB);
    EXPECT_EQ(buf.readable_bytes(), 1u);
    EXPECT_EQ(buf.read_uint8(), 0xAB);
}

TEST(BufferTest, WriteReadUint16Be) {
    Buffer buf;
    uint16_t val = 0x1234;
    buf.write_uint16_be(val);
    EXPECT_EQ(buf.read_uint16_be(), val);
}

TEST(BufferTest, WriteReadUint32Be) {
    Buffer buf;
    uint32_t val = 0x12345678;
    buf.write_uint32_be(val);
    EXPECT_EQ(buf.read_uint32_be(), val);
}

TEST(BufferTest, WriteReadUint64Be) {
    Buffer buf;
    uint64_t val = 0x123456789ABCDEF0;
    buf.write_uint64_be(val);
    EXPECT_EQ(buf.read_uint64_be(), val);
}

TEST(BufferTest, PeekDoesNotMovePointer) {
    Buffer buf;
    buf.write_uint8(0x12);
    buf.write_uint8(0x34);

    EXPECT_EQ(buf.peek_uint8(0), 0x12);
    EXPECT_EQ(buf.peek_uint8(1), 0x34);
    EXPECT_EQ(buf.readable_bytes(), 2u);
}

TEST(BufferTest, Skip) {
    Buffer buf;
    buf.write_uint8(0x01);
    buf.write_uint8(0x02);
    buf.write_uint8(0x03);
    buf.write_uint8(0x04);

    buf.skip(2);
    EXPECT_EQ(buf.readable_bytes(), 2u);
    EXPECT_EQ(buf.read_uint8(), 0x03);
}

TEST(BufferTest, Unskip) {
    Buffer buf;
    buf.write_uint8(0x01);
    buf.write_uint8(0x02);

    uint8_t val;
    buf.read(&val, 1);
    EXPECT_EQ(buf.readable_bytes(), 1u);

    buf.unskip(1);
    EXPECT_EQ(buf.readable_bytes(), 2u);
}

TEST(BufferTest, Clear) {
    Buffer buf;
    buf.write_uint8(0xAB);
    EXPECT_EQ(buf.readable_bytes(), 1u);
    buf.clear();
    EXPECT_EQ(buf.readable_bytes(), 0u);
}

TEST(BufferTest, Compact) {
    Buffer buf(1024);
    // 写入一些数据
    for (int i = 0; i < 100; ++i) {
        buf.write_uint8(static_cast<uint8_t>(i));
    }
    // 读取部分数据
    buf.skip(50);
    EXPECT_EQ(buf.readable_bytes(), 50u);
    // 压缩
    buf.compact();
    EXPECT_EQ(buf.readable_bytes(), 50u);
    // 验证数据
    for (int i = 50; i < 100; ++i) {
        EXPECT_EQ(buf.read_uint8(), static_cast<uint8_t>(i));
    }
}

TEST(BufferTest, ReserveCommit) {
    Buffer buf;
    uint8_t* ptr = buf.reserve(100);
    EXPECT_NE(ptr, nullptr);
    for (int i = 0; i < 50; ++i) {
        ptr[i] = static_cast<uint8_t>(i);
    }
    buf.commit(50);
    EXPECT_EQ(buf.readable_bytes(), 50u);
}

TEST(BufferTest, ReserveCapacity) {
    Buffer buf;
    EXPECT_TRUE(buf.reserve_capacity(65536));
    EXPECT_EQ(buf.capacity(), 65536u);
}

TEST(BufferTest, Reset) {
    Buffer buf;
    buf.write_uint8(0xAB);
    buf.reserve_capacity(65536);
    buf.reset();
    EXPECT_EQ(buf.capacity(), Buffer::DEFAULT_INITIAL_CAPACITY);
    EXPECT_EQ(buf.readable_bytes(), 0u);
}

TEST(BufferTest, MoveConstruct) {
    Buffer buf1;
    buf1.write_uint8(0xAA);
    Buffer buf2(std::move(buf1));
    EXPECT_EQ(buf2.readable_bytes(), 1u);
    EXPECT_EQ(buf2.read_uint8(), 0xAA);
}

TEST(BufferTest, MoveAssign) {
    Buffer buf1;
    buf1.write_uint8(0xAA);
    Buffer buf2;
    buf2 = std::move(buf1);
    EXPECT_EQ(buf2.readable_bytes(), 1u);
    EXPECT_EQ(buf2.read_uint8(), 0xAA);
}

// =============================================================================
// Logger模块测试用例
// =============================================================================

TEST(LoggerTest, Init) {
    Logger& logger = Logger::instance();
    int ret = logger.init(LogLevel::INFO, "");
    EXPECT_EQ(ret, 0);
}

TEST(LoggerTest, SetLevel) {
    Logger& logger = Logger::instance();
    logger.set_level(LogLevel::DEBUG);
    EXPECT_TRUE(logger.is_level_enabled(LogLevel::DEBUG));
}

TEST(LoggerTest, StringToLogLevel) {
    EXPECT_EQ(string_to_log_level("DEBUG"), LogLevel::DEBUG);
    EXPECT_EQ(string_to_log_level("INFO"), LogLevel::INFO);
    EXPECT_EQ(string_to_log_level("WARN"), LogLevel::WARN);
    EXPECT_EQ(string_to_log_level("ERROR"), LogLevel::ERROR);
}

TEST(LoggerTest, LogLevelToString) {
    EXPECT_STREQ(log_level_to_string(LogLevel::DEBUG), "DEBUG");
    EXPECT_STREQ(log_level_to_string(LogLevel::INFO), "INFO");
    EXPECT_STREQ(log_level_to_string(LogLevel::WARN), "WARN");
    EXPECT_STREQ(log_level_to_string(LogLevel::ERROR), "ERROR");
}

// =============================================================================
// LockFreeQueue模块测试用例
// =============================================================================

TEST(LockFreeQueueTest, CreateEmpty) {
    LockFreeQueue<int> q;
    EXPECT_TRUE(q.empty());
}

TEST(LockFreeQueueTest, PushPop) {
    LockFreeQueue<int> q;
    q.push(42);
    EXPECT_FALSE(q.empty());

    int val;
    EXPECT_TRUE(q.pop(val));
    EXPECT_EQ(val, 42);
    EXPECT_TRUE(q.empty());
}

TEST(LockFreeQueueTest, PopEmpty) {
    LockFreeQueue<int> q;
    int val;
    EXPECT_FALSE(q.pop(val));
}

TEST(LockFreeQueueTest, PushBatchCopy) {
    LockFreeQueue<int> q;
    std::vector<int> items = {1, 2, 3};
    q.push_batch(items);

    int val;
    EXPECT_TRUE(q.pop(val));
    EXPECT_EQ(val, 1);
    EXPECT_TRUE(q.pop(val));
    EXPECT_EQ(val, 2);
    EXPECT_TRUE(q.pop(val));
    EXPECT_EQ(val, 3);
}

TEST(LockFreeQueueTest, PushBatchMove) {
    LockFreeQueue<std::string> q;
    std::vector<std::string> items = {"a", "b", "c"};
    q.push_batch(std::move(items));

    std::string val;
    EXPECT_TRUE(q.pop(val));
    EXPECT_EQ(val, "a");
    EXPECT_TRUE(q.pop(val));
    EXPECT_EQ(val, "b");
    EXPECT_TRUE(q.pop(val));
    EXPECT_EQ(val, "c");
}

TEST(LockFreeQueueTest, PushBatchMoveIterators) {
    LockFreeQueue<std::string> q;
    std::vector<std::string> items = {"x", "y", "z"};
    q.push_batch_move(items.begin(), items.end());

    std::string val;
    EXPECT_TRUE(q.pop(val));
    EXPECT_EQ(val, "x");
}

TEST(LockFreeQueueTest, PopBatchVector) {
    LockFreeQueue<int> q;
    q.push(1);
    q.push(2);
    q.push(3);

    auto items = q.pop_batch(5);
    EXPECT_EQ(items.size(), 3u);
    EXPECT_EQ(items[0], 1);
    EXPECT_EQ(items[1], 2);
    EXPECT_EQ(items[2], 3);
}

TEST(LockFreeQueueTest, PopBatchToVector) {
    LockFreeQueue<int> q;
    q.push(1);
    q.push(2);
    q.push(3);

    std::vector<int> out;
    size_t count = q.pop_batch(out, 2);
    EXPECT_EQ(count, 2u);
    EXPECT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0], 1);
    EXPECT_EQ(out[1], 2);
}

TEST(LockFreeQueueTest, PopBatchEmpty) {
    LockFreeQueue<int> q;
    auto items = q.pop_batch(10);
    EXPECT_TRUE(items.empty());
}

// =============================================================================
// Statistics模块测试用例
// =============================================================================

TEST(StatisticsTest, RecordConnection) {
    StatisticsManager& mgr = StatisticsManager::instance();
    mgr.reset();
    mgr.record_connection();
    mgr.record_connection();
    mgr.record_connection();
    EXPECT_EQ(mgr.total_connections(), 3u);
    EXPECT_EQ(mgr.current_connections(), 3u);
}

TEST(StatisticsTest, RecordConnectionClose) {
    StatisticsManager& mgr = StatisticsManager::instance();
    mgr.reset();
    mgr.record_connection();
    mgr.record_connection();
    mgr.record_connection();
    mgr.record_connection_close();
    EXPECT_EQ(mgr.current_connections(), 2u);
}

TEST(StatisticsTest, RecordRequest) {
    StatisticsManager& mgr = StatisticsManager::instance();
    mgr.reset();
    for (int i = 0; i < 100; ++i) {
        mgr.record_request();
    }
    EXPECT_EQ(mgr.total_requests(), 100u);
}

TEST(StatisticsTest, RecordLatency) {
    StatisticsManager& mgr = StatisticsManager::instance();
    mgr.reset();
    mgr.record_latency(50);
    mgr.record_latency(100);
    Statistics stats;
    mgr.get_statistics(&stats);
    // 验证至少记录了数据（百分比计算在update后触发）
}

TEST(StatisticsTest, CalculatePercentiles) {
    StatisticsManager& mgr = StatisticsManager::instance();
    mgr.reset();
    // 记录1-100ms的延迟
    for (uint32_t i = 1; i <= 100; ++i) {
        mgr.record_latency(i);
    }
    mgr.update();

    Statistics stats;
    mgr.get_statistics(&stats);
    EXPECT_GT(stats.p50_response_latency_ms, 0u);
    EXPECT_GT(stats.p95_response_latency_ms, 0u);
    EXPECT_GT(stats.p99_response_latency_ms, 0u);
}

TEST(StatisticsTest, RecordBytes) {
    StatisticsManager& mgr = StatisticsManager::instance();
    mgr.reset();
    mgr.record_bytes_received(1024);
    mgr.record_bytes_sent(2048);

    Statistics stats;
    mgr.get_statistics(&stats);
    EXPECT_EQ(stats.total_bytes_received, 1024u);
    EXPECT_EQ(stats.total_bytes_sent, 2048u);
}

TEST(StatisticsTest, UpdateRps) {
    StatisticsManager& mgr = StatisticsManager::instance();
    mgr.reset();
    for (int i = 0; i < 100; ++i) {
        mgr.record_request();
    }
    mgr.update();

    Statistics stats;
    mgr.get_statistics(&stats);
    EXPECT_EQ(stats.requests_per_second, 100u);
}

TEST(StatisticsTest, Reset) {
    StatisticsManager& mgr = StatisticsManager::instance();
    mgr.reset();
    mgr.record_connection();
    mgr.record_request();
    mgr.record_latency(50);
    mgr.reset();

    EXPECT_EQ(mgr.total_connections(), 0u);
    EXPECT_EQ(mgr.total_requests(), 0u);
}

// =============================================================================
// Config模块测试用例
// =============================================================================

TEST(ConfigTest, CreateDefault) {
    Config cfg;
    EXPECT_EQ(cfg.get_server().listen_port, 8443);
    EXPECT_EQ(cfg.get_server().listen_ip, "0.0.0.0");
}

TEST(ConfigTest, LoadFromStringValid) {
    Config cfg;
    std::string json_str = R"({
        "server": {
            "listen_port": 9000
        }
    })";
    auto result = cfg.load_from_string(json_str);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(cfg.get_server().listen_port, 9000);
}

TEST(ConfigTest, LoadFromStringInvalid) {
    Config cfg;
    auto result = cfg.load_from_string("invalid json");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code(), ErrorCode::CONFIG_PARSE_ERROR);
}

TEST(ConfigTest, ValidateSuccess) {
    Config cfg;
    auto result = cfg.validate();
    EXPECT_TRUE(result.is_ok());
}

TEST(ConfigTest, ValidateInvalidPort) {
    Config cfg;
    ServerConfig sc;
    sc.listen_port = 0;
    cfg.set_server(sc);
    auto result = cfg.validate();
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code(), ErrorCode::CONFIG_INVALID_PORT);
}

TEST(ConfigTest, ValidateInvalidThreadCount) {
    Config cfg;
    ServerConfig sc;
    sc.thread_count = 0;
    cfg.set_server(sc);
    auto result = cfg.validate();
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code(), ErrorCode::CONFIG_INVALID_THREAD_COUNT);
}

TEST(ConfigTest, ValidateInvalidLogLevel) {
    Config cfg;
    LoggingConfig lc;
    lc.level = "INVALID";
    cfg.set_logging(lc);
    auto result = cfg.validate();
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code(), ErrorCode::CONFIG_INVALID_LOG_LEVEL);
}

TEST(ConfigTest, ToJsonString) {
    Config cfg;
    auto result = cfg.to_json_string();
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value().empty());
}

TEST(ConfigTest, MoveConstruct) {
    Config cfg1;
    cfg1.get_server();  // 访问一下确保对象有效
    Config cfg2(std::move(cfg1));
    EXPECT_EQ(cfg2.get_server().listen_port, 8443);
}

TEST(ConfigTest, SetAndGet) {
    Config cfg;

    ServerConfig sc;
    sc.listen_port = 9001;
    cfg.set_server(sc);
    EXPECT_EQ(cfg.get_server().listen_port, 9001);

    DebugConfig dc;
    dc.enabled = true;
    cfg.set_debug(dc);
    EXPECT_TRUE(cfg.get_debug().enabled);

    Http2Config hc;
    hc.enabled = false;
    cfg.set_http2(hc);
    EXPECT_FALSE(cfg.get_http2().enabled);
}

// 文件结束
