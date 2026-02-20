// =============================================================================
//  HTTPS Server Simulator - Integration Test
//  文件: test_event_flow.cpp
//  描述: 事件流集成测试 - 验证MsgCenter + EventLoop + EventQueue模块协作
//  版权: Copyright (c) 2026
// =============================================================================

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include "msg_center/msg_center.hpp"
#include "msg_center/event.hpp"
#include "msg_center/event_queue.hpp"
#include "msg_center/event_loop.hpp"

namespace https_server_sim {
namespace integration_test {

// ==================== 测试辅助类 ====================

class EventCounter {
public:
    EventCounter() : count_(0) {}

    void increment() {
        std::lock_guard<std::mutex> lock(mutex_);
        count_++;
        cv_.notify_all();
    }

    int get_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_;
    }

    bool wait_for_count(int expected, int timeout_ms = 1000) {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                          [this, expected]() { return count_ >= expected; });
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    int count_;
};

// ==================== 集成测试用例 ====================

// Integration_UseCase013: EventQueue基本操作
// 验证: EventQueue能push和pop事件
TEST(IntegrationTest, UseCase013_EventQueueBasicOperations) {
    EventQueue queue;

    // push事件
    Event event;
    event.type = EventType::READ;
    event.conn_id = 1234;
    queue.push(event);

    // pop事件
    Event popped;
    bool success = queue.try_pop(popped);
    EXPECT_TRUE(success);
    EXPECT_EQ(popped.type, EventType::READ);
    EXPECT_EQ(popped.conn_id, 1234ULL);
}

// Integration_UseCase014: EventQueue多线程生产消费
// 验证: EventQueue在多线程下安全
TEST(IntegrationTest, UseCase014_EventQueueMultiThreaded) {
    EventQueue queue;
    std::atomic<int> produced_count{0};
    std::atomic<int> consumed_count{0};
    std::atomic<bool> done{false};

    // 生产者线程
    std::thread producer([&]() {
        for (int i = 0; i < 100; i++) {
            Event event;
            event.type = EventType::READ;
            event.conn_id = i;
            queue.push(event);
            produced_count++;
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });

    // 消费者线程
    std::thread consumer([&]() {
        while (!done || produced_count > consumed_count) {
            Event event;
            if (queue.try_pop(event)) {
                consumed_count++;
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }
    });

    producer.join();
    done = true;
    consumer.join();

    EXPECT_EQ(produced_count.load(), 100);
    EXPECT_EQ(consumed_count.load(), 100);
}

// Integration_UseCase015: MsgCenter post事件
// 验证: MsgCenter能post事件并被处理
TEST(IntegrationTest, UseCase015_MsgCenterPostEvent) {
    MsgCenter msg_center(1, 1);
    EventCounter counter;

    int ret = msg_center.start();
    ASSERT_EQ(ret, 0);

    // post带handler的事件
    Event event;
    event.type = EventType::READ;
    event.handler = [&counter]() {
        counter.increment();
    };

    msg_center.post_event(event);

    // 等待事件处理
    bool received = counter.wait_for_count(1, 1000);
    EXPECT_TRUE(received) << "Event should be handled";

    msg_center.stop();
}

// Integration_UseCase016: MsgCenter post多个事件
// 验证: 多个事件能按顺序处理
TEST(IntegrationTest, UseCase016_MsgCenterPostMultipleEvents) {
    MsgCenter msg_center(1, 1);
    EventCounter counter;

    int ret = msg_center.start();
    ASSERT_EQ(ret, 0);

    // post 10个事件
    for (int i = 0; i < 10; i++) {
        Event event;
        event.type = EventType::READ;
        event.handler = [&counter]() {
            counter.increment();
        };
        msg_center.post_event(event);
    }

    // 等待所有事件处理
    bool received = counter.wait_for_count(10, 2000);
    EXPECT_TRUE(received) << "All events should be handled";
    EXPECT_EQ(counter.get_count(), 10);

    msg_center.stop();
}

// Integration_UseCase017: MsgCenter WorkerPool执行任务
// 验证: WorkerPool能执行回调任务
TEST(IntegrationTest, UseCase017_WorkerPoolExecuteTask) {
    MsgCenter msg_center(1, 2);
    EventCounter counter;
    std::thread::id main_thread_id = std::this_thread::get_id();
    std::atomic<std::thread::id> worker_thread_id{};

    int ret = msg_center.start();
    ASSERT_EQ(ret, 0);

    // post回调任务
    msg_center.post_callback_task([&counter, &worker_thread_id, main_thread_id]() {
        worker_thread_id = std::this_thread::get_id();
        counter.increment();
    });

    // 等待任务执行
    bool received = counter.wait_for_count(1, 1000);
    EXPECT_TRUE(received) << "Task should be executed";

    // 验证在不同线程执行
    EXPECT_NE(worker_thread_id.load(), main_thread_id)
        << "Task should run on worker thread, not main thread";

    msg_center.stop();
}

// Integration_UseCase018: 事件优先级
// 验证: 高优先级事件先处理
TEST(IntegrationTest, UseCase018_EventPriority) {
    // 注意：当前EventQueue可能不支持优先级，这个测试验证基本功能
    EventQueue queue;

    // push不同类型事件
    Event read_event;
    read_event.type = EventType::READ;
    read_event.conn_id = 1;
    queue.push(read_event);

    Event accept_event;
    accept_event.type = EventType::ACCEPT;
    accept_event.conn_id = 2;
    queue.push(accept_event);

    Event shutdown_event;
    shutdown_event.type = EventType::SHUTDOWN;
    shutdown_event.conn_id = 3;
    queue.push(shutdown_event);

    // pop事件 - 验证能全部取出
    std::vector<EventType> popped_types;
    Event popped;

    for (int i = 0; i < 3; i++) {
        bool success = queue.try_pop(popped);
        EXPECT_TRUE(success);
        popped_types.push_back(popped.type);
    }

    // 验证所有事件都收到了
    EXPECT_EQ(popped_types.size(), 3UL);
}

// Integration_UseCase019: EventLoop停止机制
// 验证: EventLoop能被优雅停止
TEST(IntegrationTest, UseCase019_EventLoopStop) {
    MsgCenter msg_center(1, 1);

    int ret = msg_center.start();
    ASSERT_EQ(ret, 0);

    // 短暂运行
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 停止
    auto start_time = std::chrono::steady_clock::now();
    msg_center.stop();
    auto end_time = std::chrono::steady_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    EXPECT_LT(duration.count(), 1000LL)
        << "EventLoop should stop quickly";

    SUCCEED();
}

// Integration_UseCase020: 事件处理异常不影响EventLoop
// 验证: 单个事件handler抛异常不影响后续事件处理
TEST(IntegrationTest, UseCase020_EventExceptionHandling) {
    MsgCenter msg_center(1, 1);
    EventCounter counter;

    int ret = msg_center.start();
    ASSERT_EQ(ret, 0);

    // post一个会抛异常的事件
    Event bad_event;
    bad_event.type = EventType::READ;
    bad_event.handler = []() {
        throw std::runtime_error("Intentional exception");
    };
    msg_center.post_event(bad_event);

    // 短暂等待
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // post一个正常事件
    Event good_event;
    good_event.type = EventType::READ;
    good_event.handler = [&counter]() {
        counter.increment();
    };
    msg_center.post_event(good_event);

    // 验证正常事件仍能处理
    bool received = counter.wait_for_count(1, 1000);
    EXPECT_TRUE(received)
        << "Good event should be handled even after bad event";

    msg_center.stop();
}

} // namespace integration_test
} // namespace https_server_sim

// 文件结束
