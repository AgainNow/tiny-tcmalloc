#pragma once

#include <iostream>
#include <forward_list>
#include <utility>
#include <atomic>
#include <cstdint>

#include "LockFreeList.h"
#include "ThreadCache.h"
#include "CentralCache.h"

namespace mem {

// 内存池
class MemoryPool {
public:
    static void* allocate(size_t size) {
        return ThreadCache::get_instance()->allocate(size);
    }

    static void deallocate(void* ptr, size_t size) {
        return ThreadCache::get_instance()->deallocate(ptr, size);
    }
};

template<typename T, typename... Args>
T* newObj(Args&&... args) {
    T* p = nullptr;
    // 申请内存
    p = (T*)MemoryPool::allocate(sizeof(T));
    // 构建对象
    if (p)
        // 在已分配的内存上创建对象
        new(p) T(std::forward<Args>(args)...);

    return p;
}

template <typename T, typename... Args>
void delObj(T* p) {
    if (p) {
        // 析构对象
        p->~T();
        // 回收内存
        MemoryPool::deallocate((void*)p, sizeof(T));
    }
}

}