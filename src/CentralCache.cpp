#include "CentralCache.h"
#include "PageCache.h"
#include "Common.h"

namespace mem {

CentralBucket::CentralBucket(size_t element_size) {
    // _free_list = new LockFreeList{};
    _element_size = element_size;
}

CentralBucket::~CentralBucket() {
    // delete _free_list;
    // _free_list = nullptr;
}

// void* CentralBucket::allocate(int t) {
//     // 优先查询空闲链表
//     Slot* slot = _free_list->pop_front(t);
//     if (slot) return slot;
//     return nullptr;
// }

// void CentralBucket::deallocate(void* ptr) {
//     if (!ptr) return;
    
//     _free_list->push_front((Slot*) ptr);
// }

// 自旋锁版本
void* CentralBucket::allocate_spinlock() {
    // 自旋锁
    while (_spinlock.test_and_set(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    // 1. 优先查询free_list
    void* head = _spinlock_free_list.load(std::memory_order_relaxed);
    if (head) {
        void* next = *(void**)head;
        _spinlock_free_list.store(next, std::memory_order_release);
        next = nullptr;
    } else {
        // 2. 若为空，则从页缓存获取大内存块，切分后插入free_list
        // 计算实际需要的页数，向上取整
        size_t page_num = (_element_size + PAGE_SIZE - 1) / PAGE_SIZE;
        // 单次申请span，最小32kb
        page_num = std::max(MIN_SPAN_SIZE, page_num);
        void* span = PageCache::get_instance()->allocate_span(page_num);
        // void* span = fetch_from_page_cache(_element_size);
        if (span) {
            size_t element_num = (page_num * PAGE_SIZE) / _element_size;
        
            // 头插法，把block每个节点串起来
            head = span;
            *(void**)head = nullptr;
            for (int i = 1; i < element_num; ++i) {
                void* next = (void*)((char*)span + i * _element_size);
                *(void**)next = head;
                head = next;
            }
            void* next = *(void**)head;
            _spinlock_free_list.store(next, std::memory_order_release);
            next = nullptr;
        }
        
    }
    // 释放锁
    _spinlock.clear(std::memory_order_release);
    return head;
}

void CentralBucket::deallocate_spinlock(void* beg, void* end, size_t size) {
    if (!beg || !end) return;
    while (_spinlock.test_and_set(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    // 头插法，如果不是spinlock，则可以考虑插入尾部，以减少和alloc部分的锁竞争
    void* head = _spinlock_free_list.load(std::memory_order_relaxed);

    *(void**)end = head;

    _spinlock_free_list.store(beg, std::memory_order_release);

    // TODO 缺少返回给PageCache的策略

    // 释放锁
    _spinlock.clear(std::memory_order_release);
}

void* CentralBucket::fetch_from_page_cache(size_t size) {
    // 计算实际需要的页数，向上取整
    size_t page_num = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    // 单次申请span，最小32kb
    page_num = std::max(MIN_SPAN_SIZE, page_num);
    return PageCache::get_instance()->allocate_span(page_num);
}

CentralCache::CentralCache() {
    for (int idx = 0; idx < FREE_LIST_SIZE; ++idx) {
        size_t alignedsize = SizeClass::index_to_alignedsize(idx);
        _buckets[idx] = new CentralBucket(alignedsize);
    }
}

CentralCache::~CentralCache() {
    for (int idx = 0; idx < FREE_LIST_SIZE; ++idx) {
        delete _buckets[idx];
        _buckets[idx] = nullptr;
    }
}

void* CentralCache::allocate(size_t size) {
    size_t idx = SizeClass::get_index(size);
    return _buckets[idx]->allocate_spinlock();
}

void CentralCache::batch_deallocate(void* beg, void* end, size_t size) {
    if (!beg || !end) return;
    
    size_t idx = SizeClass::get_index(size);
    _buckets[idx]->deallocate_spinlock(beg, end, size);
}

}