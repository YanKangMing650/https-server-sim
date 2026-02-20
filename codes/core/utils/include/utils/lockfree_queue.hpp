#pragma once

#include <atomic>
#include <utility>
#include <vector>

namespace https_server_sim {
namespace utils {

// 单生产者单消费者(SPSC)无锁队列 - 链表实现
//
// 内存管理策略说明：
// - 使用哨兵节点简化边界条件
// - 消费者线程(pop/pop_batch)负责释放出队节点内存
// - SPSC场景保证：同一时间只有一个线程在释放内存
// - 析构函数删除所有剩余节点
//
// 内存序选择说明：
// - tail_.exchange使用memory_order_acq_rel：发布新tail，获取旧tail
// - next.store使用memory_order_release：发布next指针
// - next.load使用memory_order_acquire：获取next指针
// - head_.store使用memory_order_release：发布新head
// - 其他操作使用memory_order_relaxed：仅单线程访问
//
template<typename T>
class LockFreeQueue {
public:
    LockFreeQueue();
    ~LockFreeQueue();

    // 禁止拷贝
    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;

    // ========== 单元素操作 ==========

    // 入队（单生产者线程调用）
    // item: 要入队的元素（右值引用，支持移动语义）
    void push(T item);

    // 出队（单消费者线程调用）
    // item: [out] 出队元素
    // return: true-成功，false-队列空
    bool pop(T& item);

    // ========== 批量操作 ==========

    // 批量入队（单生产者线程调用）- 复制版本
    template<typename InputIt>
    void push_batch(InputIt first, InputIt last);

    // 批量入队（单生产者线程调用）- 移动版本
    template<typename InputIt>
    void push_batch_move(InputIt first, InputIt last);

    // 批量入队（vector重载）- 复制版本
    void push_batch(const std::vector<T>& items);

    // 批量入队（vector重载）- 移动版本
    void push_batch(std::vector<T>&& items);

    // 批量出队（单消费者线程调用）
    // max_count: 最多出队的元素数量
    // return: 出队的元素vector，可能少于max_count
    std::vector<T> pop_batch(size_t max_count);

    // 批量出队到vector（效率更高）
    // out: [out] 输出vector，元素会追加到末尾
    // max_count: 最多出队的元素数量
    // return: 实际出队的元素数量
    size_t pop_batch(std::vector<T>& out, size_t max_count);

    // ========== 状态检查 ==========

    // 检查是否为空（仅消费者线程调用）
    bool empty() const;

private:
    // 节点基类（用于哨兵节点，不包含数据）
    struct NodeBase {
        std::atomic<NodeBase*> next;
        NodeBase() : next(nullptr) {}
        virtual ~NodeBase() = default;
    };

    // 数据节点（包含实际数据）
    struct Node : NodeBase {
        T data;
        Node(T d) : NodeBase(), data(std::move(d)) {}
    };

    std::atomic<NodeBase*> head_;
    std::atomic<NodeBase*> tail_;
};

// ========== 实现 ==========

template<typename T>
LockFreeQueue<T>::LockFreeQueue() {
    // 初始哨兵节点（使用基类，不要求T可默认构造）
    NodeBase* dummy = new NodeBase();
    head_.store(dummy, std::memory_order_relaxed);
    tail_.store(dummy, std::memory_order_relaxed);
}

template<typename T>
LockFreeQueue<T>::~LockFreeQueue() {
    // 删除所有节点（仅单线程调用析构，无竞争）
    NodeBase* curr = head_.load(std::memory_order_relaxed);
    while (curr != nullptr) {
        NodeBase* next = curr->next.load(std::memory_order_relaxed);
        // 注意：只有数据节点是Node*类型，哨兵节点是NodeBase*
        // 但我们统一用NodeBase*删除，因为析构函数是平凡的
        delete curr;
        curr = next;
    }
}

template<typename T>
void LockFreeQueue<T>::push(T item) {
    Node* new_node = new Node(std::move(item));
    new_node->next.store(nullptr, std::memory_order_relaxed);

    // SPSC正确顺序：
    // 1. 先获取当前tail
    NodeBase* old_tail = tail_.load(std::memory_order_relaxed);

    // 2. 关键：先链接old_tail->next（release保证后续操作可见）
    old_tail->next.store(new_node, std::memory_order_release);

    // 3. 再更新tail（relaxed，因为只有生产者线程修改）
    tail_.store(new_node, std::memory_order_relaxed);
}

template<typename T>
bool LockFreeQueue<T>::pop(T& item) {
    NodeBase* old_head = head_.load(std::memory_order_relaxed);
    NodeBase* next = old_head->next.load(std::memory_order_acquire);

    if (next == nullptr) {
        return false;
    }

    // 取出数据（next是数据节点，安全转换为Node*）
    Node* data_node = static_cast<Node*>(next);
    item = std::move(data_node->data);

    // 更新head，释放old_head
    head_.store(next, std::memory_order_release);
    delete old_head;

    return true;
}

template<typename T>
template<typename InputIt>
void LockFreeQueue<T>::push_batch(InputIt first, InputIt last) {
    if (first == last) {
        return;
    }

    // 构建局部链表 - 复制元素
    Node* batch_head = new Node(*first++);
    Node* batch_tail = batch_head;

    for (auto it = first; it != last; ++it) {
        Node* new_node = new Node(*it);
        batch_tail->next.store(new_node, std::memory_order_relaxed);
        batch_tail = new_node;
    }

    batch_tail->next.store(nullptr, std::memory_order_relaxed);

    // SPSC正确顺序：先链接，再更新tail
    NodeBase* old_tail = tail_.load(std::memory_order_relaxed);
    old_tail->next.store(batch_head, std::memory_order_release);
    tail_.store(batch_tail, std::memory_order_relaxed);
}

template<typename T>
template<typename InputIt>
void LockFreeQueue<T>::push_batch_move(InputIt first, InputIt last) {
    if (first == last) {
        return;
    }

    // 构建局部链表 - 移动元素
    Node* batch_head = new Node(std::move(*first++));
    Node* batch_tail = batch_head;

    for (auto it = first; it != last; ++it) {
        Node* new_node = new Node(std::move(*it));
        batch_tail->next.store(new_node, std::memory_order_relaxed);
        batch_tail = new_node;
    }

    batch_tail->next.store(nullptr, std::memory_order_relaxed);

    // SPSC正确顺序：先链接，再更新tail
    NodeBase* old_tail = tail_.load(std::memory_order_relaxed);
    old_tail->next.store(batch_head, std::memory_order_release);
    tail_.store(batch_tail, std::memory_order_relaxed);
}

template<typename T>
void LockFreeQueue<T>::push_batch(const std::vector<T>& items) {
    push_batch(items.begin(), items.end());
}

template<typename T>
void LockFreeQueue<T>::push_batch(std::vector<T>&& items) {
    if (items.empty()) {
        return;
    }

    // 构建局部链表（直接从vector移动）
    Node* batch_head = new Node(std::move(items[0]));
    Node* batch_tail = batch_head;

    for (size_t i = 1; i < items.size(); ++i) {
        Node* new_node = new Node(std::move(items[i]));
        batch_tail->next.store(new_node, std::memory_order_relaxed);
        batch_tail = new_node;
    }

    batch_tail->next.store(nullptr, std::memory_order_relaxed);

    // SPSC正确顺序：先链接，再更新tail
    NodeBase* old_tail = tail_.load(std::memory_order_relaxed);
    old_tail->next.store(batch_head, std::memory_order_release);
    tail_.store(batch_tail, std::memory_order_relaxed);
}

template<typename T>
std::vector<T> LockFreeQueue<T>::pop_batch(size_t max_count) {
    std::vector<T> result;
    result.reserve(max_count);
    pop_batch(result, max_count);
    return result;
}

template<typename T>
size_t LockFreeQueue<T>::pop_batch(std::vector<T>& out, size_t max_count) {
    size_t count = 0;

    NodeBase* old_head = head_.load(std::memory_order_relaxed);
    NodeBase* first_data_node = nullptr;
    NodeBase* last_data_node = nullptr;

    // ========== 第一阶段：遍历链表收集数据（不释放内存） ==========
    // 注意：在SPSC场景下，此时只有消费者线程在读next指针
    // 生产者可能在追加新节点，但不会修改已存在节点的next指针
    NodeBase* curr = old_head;
    while (count < max_count) {
        NodeBase* next = curr->next.load(std::memory_order_acquire);
        if (next == nullptr) {
            break;  // 队列已空
        }

        // 记录第一个数据节点（用于后续释放old_head）
        if (first_data_node == nullptr) {
            first_data_node = next;
        }
        last_data_node = next;

        // 取出数据（next是数据节点，安全转换为Node*）
        Node* data_node = static_cast<Node*>(next);
        out.push_back(std::move(data_node->data));
        curr = next;
        ++count;
    }

    // ========== 第二阶段：更新head并释放内存 ==========
    if (count > 0) {
        // 更新head为最后一个数据节点（成为新的哨兵节点）
        head_.store(last_data_node, std::memory_order_release);

        // 批量释放旧节点：从old_head到first_data_node之前的节点
        // 注意：old_head是原哨兵节点，first_data_node是第一个数据节点
        // 释放流程：old_head -> ... -> first_data_node的前一个节点
        NodeBase* node_to_delete = old_head;
        while (node_to_delete != last_data_node) {
            NodeBase* next_node = node_to_delete->next.load(std::memory_order_relaxed);
            delete node_to_delete;
            node_to_delete = next_node;
        }
    }

    return count;
}

template<typename T>
bool LockFreeQueue<T>::empty() const {
    NodeBase* head = head_.load(std::memory_order_relaxed);
    NodeBase* next = head->next.load(std::memory_order_acquire);
    return next == nullptr;
}

} // namespace utils
} // namespace https_server_sim
