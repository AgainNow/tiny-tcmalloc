#pragma once

#include "CentralCache.h"
#include "Common.h"

#include <list>

namespace mem {
// 线程本地缓存
// 策略：
//     1. 每个线程缓存n条空闲链表，每条链表的节点大小不同；每条链表大小不超过64，超过时，返回3/4给中心缓存
class ThreadCache {
public:
    static ThreadCache* get_instance() {
        static thread_local ThreadCache inst;
        return &inst;
    }

public:
    ThreadCache();
    ~ThreadCache() = default;

public:
    void* allocate(size_t size);
    void deallocate(void* ptr, size_t size);

private:
    // 本地缓存
    // 性能优化点：不使用STL，避免不必要开销；预分配足量内存；直接使用void*裸指针，不定义额外结构体如Slot*
    std::array<void*, FREE_LIST_SIZE> _free_list;  // idx: 内存块链表
    std::array<size_t, FREE_LIST_SIZE> _free_list_size;  // idx: 对应内存块链表大小

};

}

