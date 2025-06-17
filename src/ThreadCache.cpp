#include "MemoryPool.h"

namespace mem {

ThreadCache::ThreadCache() {
    _free_list.fill(nullptr);
    _free_list_size.fill(0);
}

size_t ThreadCache::get_batch_num(size_t size) {
    // 策略：
    //     1. 每个批次总大小不超过2kb，单位内存大小超过2kb的除外
    //     2. 批次最少1个，最多64个
    size_t batch_num = 1;
    if (size <= 32) batch_num = 64;
    else if (size <= 64) batch_num = 32;
    else if (size <= 128) batch_num = 16;
    else if (size <= 256) batch_num = 8;
    else if (size <= 512) batch_num = 4;
    else if (size <= 1024) batch_num = 2;

    return batch_num;
}

void* ThreadCache::allocate(size_t size) {
    if (size <= 0) return nullptr;
    if (size > MAX_BYTES)
        return operator new(size);
    
    // 优先从线程本地自由链表里取
    size_t idx = SizeClass::get_index(size);
    if (void* ptr = _free_list[idx]) {
        _free_list_size[idx]--;  // 更新大小
        _free_list[idx] = *(void**)ptr;  // 指向next节点
        return ptr;
    }

    // 如果本地不足，再向缓存中心批量申请
    // TODO 其他策略：是否考虑改成批量申请，或增量时申请，比如根据当前在用的内存量，1.5倍或2倍申请，以达到快速满足线程需要的目的
    size_t alignedsize = SizeClass::get_alignedsize(size);
    size_t batch_num = get_batch_num(alignedsize);
    void* head =  CentralCache::get_instance().batch_allocate(alignedsize, batch_num);
    _free_list[idx] = *(void**)head;
    _free_list_size[idx] = batch_num - 1;
    *(void**)head = nullptr;
    return head;
}

void ThreadCache::deallocate(void* ptr, size_t size) {
    if (!ptr || size <= 0) return;
    if (size > MAX_BYTES) {
        operator delete(ptr);
        return;
    }
    
    // 优先插入线程本地自由链表
    size_t idx = SizeClass::get_index(size);
    *(void**)ptr = _free_list[idx];
    _free_list[idx] = ptr;
    _free_list_size[idx]++;

    // 若链表长度超过64，则返还3/4给中心缓存
    if (_free_list_size[idx] > TC_MAX_LIST_SIZE) {
        // 申请时size做过对齐，释放时也需要对齐
        size_t aligned_size = SizeClass::get_alignedsize(size);
        // 需要返还的内存块数量
        size_t return_num = _free_list_size[idx] / 4 * 3;
        _free_list_size[idx] = _free_list_size[idx] - return_num;

        void* beg = _free_list[idx];
        void* end = beg;
        while (--return_num) {
            end = *(void**)end;
        }
        _free_list[idx] = *(void**)end;
        
        CentralCache::get_instance().batch_deallocate(beg, end, aligned_size);
    }
}
    
}
