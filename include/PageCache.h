#pragma once

#include <forward_list>
#include <mutex>
#include <map>
#include "LockFreeList.h"

namespace mem {

// 页缓存
class PageCache {
public:
    // static const size_t PAGE_SIZE = 4096;  // 默认页大小 4k
public:
    PageCache(size_t slot_size, size_t block_size = 4096);
    ~PageCache();

public:
    static PageCache* get_instance() {
        static PageCache inst(0);
        return &inst;
    }

public:
    // 分配指定页数的span
    void* allocate_span(size_t page_num);

    void deallocate_span(void* ptr, size_t page_num);

private:
    // 通过mmap系统调用申请按页申请内存
    void* system_alloc(size_t page_num);

public:
    // void* allocate();  // 每次返回一个Slot大小的内存

public:
    // void* allocate_block();  // 每次返回一个Block大小的内存，配合spinlock使用

private:
    // void allocate_operator_new();  // 从向系统申请内存块

    // void allocate_mmap();

private:
    struct Span {  // n个page的集合，用来管理page
        void* page_addr;  // 页起始地址
        size_t page_num;  // 页数
        Span* next;
    };

    // 按页数管理空闲span，不同页数对应不同的span链表
    std::map<size_t, Span*> _free_span;
    // 页起始地址（页号）到span的映射，用于回收（所有的span内存都会记录，包括空闲的和使用中的）
    std::map<void*, Span*> _all_span_record;
private:
    // size_t _element_size;

    // std::forward_list<void*> _free_list;  // block链表
    // size_t _size;

    // Slot* _cur_slot;  // 当前block下一块可用内存槽
    // Slot* _end_slot;  // 当前block结束位置

    std::mutex _mtx;  // 锁
};

}