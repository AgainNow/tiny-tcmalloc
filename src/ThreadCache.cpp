#include "MemoryPool.h"

namespace mem {

ThreadCache::ThreadCache() {
    _free_list.fill(nullptr);
    _free_list_size.fill(0);
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

    // 如果本地不足，再向缓存中心申请（目前每次申请一块）
    // TODO 是否考虑改成批量申请，或增量时申请，比如根据当前在用的内存量，1.5倍或2倍申请，以达到快速满足线程需要的目的
    size_t aligned_size = SizeClass::get_alignedsize(size);
    return CentralCache::get_instance().allocate(aligned_size);
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
    if (_free_list_size[idx] > 64) {
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
