#pragma once

#include <forward_list>
#include <list>
#include <array>

#include "LockFreeList.h"
#include "PageCache.h"
#include "Common.h"

namespace mem {

// 中心缓存中的桶
class CentralBucket {
public:
    // @element_size 桶内内存块大小
    CentralBucket(size_t element_size);
    ~CentralBucket();
public:
    // 向内存池申请内存
    // void* allocate(int t = 0);
    // 释放内存
    // void deallocate(void* ptr);
public:
    // @element_num 需要获取的内存块数量
    void* allocate_spinlock(size_t& element_num);
    // 批量插入
    void deallocate_spinlock(void* beg, void* end, size_t size);
private:
    std::atomic_flag _spinlock;  // 自旋锁
    std::atomic<void*> _spinlock_free_list;  // TODO 既然都已经用自旋锁锁上了，为什么还要用原子变量？
private:
    // LockFreeList* _free_list;  // 空闲内存槽链表

    size_t _element_size;  // 桶中内存块的大小
};

// 中心缓存
class CentralCache {
private:
    CentralCache();
    ~CentralCache();
public:
    static CentralCache& get_instance() {
        static CentralCache m;
        return m;
}
public:
    // 申请不大于batch_num块大小为size的内存块
    // @batch_num 必须大于等于1
    void* batch_allocate(size_t size, size_t& batch_num);
    // 批量释放内存块
    // @beg 有效头结点；@end 有效尾节点
    void batch_deallocate(void* beg, void* end, size_t size);
private:
    std::array<CentralBucket*, FREE_LIST_SIZE> _buckets;
};

}