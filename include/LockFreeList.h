#pragma once

#include <atomic>
#include <iostream>
#include <thread>
#include <chrono>

namespace mem {

// 内存槽
struct Slot {
    Slot(): next(nullptr) {}
    Slot* next;
};

// 指针+版本号组合，避免 ABA
struct TaggedPtr {
    Slot* ptr;
    uint64_t count;

    bool operator==(const TaggedPtr& other) const {
        return ptr == other.ptr && count == other.count;
    }
};

// 带版本号的无锁队列
class LockFreeList {
private:
    std::atomic<TaggedPtr> head;

public:
    LockFreeList() {
        head.store({nullptr, 0});
    }

    // 线程安全 + 防 ABA 的头插法
    // @t time，测试参数，非测试场景请勿使用
    void push_front(Slot* node, int t = 0) {
        TaggedPtr old_head;
        TaggedPtr new_head;

        do {
            // 要求该指令之后的指令不能重排序到它之前
            old_head = head.load(std::memory_order_acquire);
            node->next = old_head.ptr;
            new_head = {node, old_head.count + 1};
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
        } while (!head.compare_exchange_weak(old_head, new_head,
                                             std::memory_order_release,  // 比较成功时，交换操作之前的指令不能排到它之后
                                             std::memory_order_acquire));  // 比较失败时，重载操作之后的指令不能排到它之前
    }

    // 线程安全 + 防 ABA 的头删除
    // @t time，测试参数，非测试场景请勿使用
    Slot* pop_front(int t = 0) {
        TaggedPtr old_head;
        TaggedPtr new_head;

        do {
            // 要求该指令之后的指令不能重排序到它之前
            old_head = head.load(std::memory_order_acquire);
            if (!old_head.ptr) return nullptr;
            new_head = {old_head.ptr->next, old_head.count + 1};
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
        } while (!head.compare_exchange_weak(old_head, new_head,
                                             std::memory_order_release,
                                             std::memory_order_acquire));
        return old_head.ptr;
    }

    // 非线程安全打印（调试用）
    void print() {
        auto cur = head.load().ptr;
        while (cur) {
            std::cout << cur << " -> ";
            cur = cur->next;
        }
        std::cout << "nullptr\n";
    }

    // 非线程安全（调试用）
    size_t size() {
        size_t s = 0;
        auto cur = head.load().ptr;
        while (cur) {
            s++;
            cur = cur->next;
        }
        return s;
    }
};

// 存在ABA问题的无锁队列
class ABALockFreeList {
private:
    std::atomic<Slot*> head;

public:
    ABALockFreeList() {
        head.store(nullptr);
    }

    // 头插法
    // @t time，测试参数，非测试场景请勿使用
    void push_front(Slot* node, int t = 0) {
        Slot* old_head;

        do {
            old_head = head.load(std::memory_order_acquire);
            node->next = old_head;
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
        } while (!head.compare_exchange_weak(old_head, node,
                                             std::memory_order_release,
                                             std::memory_order_acquire));
    }

    // 头删除
    // @t time，测试参数，非测试场景请勿使用
    Slot* pop_front(int t = 0) {
        Slot* old_head;
        Slot* new_head;

        do {
            old_head = head.load(std::memory_order_acquire);
            if (!old_head) return nullptr;
            new_head = old_head->next;
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
        } while (!head.compare_exchange_weak(old_head, new_head,
                                             std::memory_order_release,
                                             std::memory_order_acquire));
        return old_head;
    }

    // 非线程安全打印（调试用）
    void print() {
        Slot* cur = head.load();
        while (cur) {
            std::cout << cur << " -> ";
            cur = cur->next;
        }
        std::cout << "nullptr\n";
    }

    // 非线程安全（调试用）
    size_t size() {
        size_t s = 0;
        auto cur = head.load();
        while (cur) {
            s++;
            cur = cur->next;
        }
        return s;
    }
};

}