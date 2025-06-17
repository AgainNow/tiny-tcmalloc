#include "CentralCache.h"
#include "PageCache.h"
#include "Common.h"

namespace mem {

CentralBucket::CentralBucket(size_t element_size) {
    // _free_list = new LockFreeList{};
    _element_size = element_size;
    _spinlock_free_list.store(nullptr, std::memory_order_relaxed);
    _spinlock.clear();
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
void* CentralBucket::allocate_spinlock(size_t& batch_num) {
    // 两种方案：
    // 1. 懒汉模式：中心缓存有多少给多少，少于batch_num也可以（目前采用）
    //     优缺点：实现简单；不涉及页缓存，减少锁触发频率
    //     实现：需要返回实际的batch_num
    // 2. 饿汉模式：一定要给足，如果中心缓存不够，向页缓存申请
    //     优缺点：特别依赖线程缓存策略的效率
    //     实现1：先遍历中心缓存的；如果不够，再申请页缓存的；多余的存进中心缓存
    //     实现2：中心缓存记录当前缓存数量；直接判断中心缓存够不够，如果不够，直接申请页缓存；多余的存到中心缓存

    // 自旋锁
    while (_spinlock.test_and_set(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    // 1. 优先从free_list中取
    void* head = _spinlock_free_list.load(std::memory_order_relaxed);
    if (head) {

        size_t count = 0;
        void* curr = head;
        void* prev = nullptr;
        while (count < batch_num && curr) {
            prev = curr;
            curr = *(void**)curr;
            count++;
        }

        batch_num = count;  // 返回真实数量
        if (prev) {
            *(void**)prev = nullptr;
        }
        _spinlock_free_list.store(curr, std::memory_order_release);

        // void* next = *(void**)head;
        // _spinlock_free_list.store(next, std::memory_order_release);
        // next = nullptr;
    } else {

        // 2. 若为空，则从页缓存获取大内存块，切分后插入free_list
        // 计算实际需要的页数，向上取整
        size_t page_num = (_element_size * batch_num + PAGE_SIZE - 1) / PAGE_SIZE;
        // 单次申请span，最小32kb
        page_num = std::max(MIN_SPAN_PAGE_NUM, page_num);
        void* span = PageCache::get_instance()->allocate_span(page_num);
        if (span) {
            size_t element_num = (page_num * PAGE_SIZE) / _element_size;
        
            // 头插法，把block每个节点串起来
            head = nullptr;
            // 先把需要返回的批次串起来
            for (int i = 0; i < batch_num; ++i) {
                void* next = (void*)((char*)span + i * _element_size);
                *(void**)next = head;
                head = next;
            }
            
            // 再把需要存入中心缓存的内存串起来
            void* extra_head = nullptr;
            for (int i = batch_num; i < element_num; ++i) {
                void* next = (void*)((char*)span + i * _element_size);
                *(void**)next = extra_head;
                extra_head = next;
            }
            _spinlock_free_list.store(extra_head, std::memory_order_release);
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

void* CentralCache::batch_allocate(size_t size, size_t& batch_num) {
    size_t idx = SizeClass::get_index(size);
    return _buckets[idx]->allocate_spinlock(batch_num);
}

void CentralCache::batch_deallocate(void* beg, void* end, size_t size) {
    if (!beg || !end) return;
    
    size_t idx = SizeClass::get_index(size);
    _buckets[idx]->deallocate_spinlock(beg, end, size);
}

}