#include "../include/MemoryPool.h"

namespace mem {

MemoryPool::MemoryPool(size_t slot_size, size_t block_size): _block_size(block_size), _slot_size(slot_size) {
    _block_cur_slot = nullptr;
    _block_end_slot = nullptr;

    _free_list = new LockFreeList{};
}

MemoryPool::~MemoryPool() {
    delete _free_list;
    _free_list = nullptr;

    for (auto& block: _block_list) {
        operator delete(block);
        block = nullptr;
    }
}

void* MemoryPool::allocate(int t) {
    // 优先查询空闲链表
    Slot* slot = _free_list->pop_front(t);
    if (slot) return slot;

    Slot* temp;
    {
        std::lock_guard<std::mutex> lock(_mtx_for_block);
        // 其次查询block
        if (_block_cur_slot >= _block_end_slot) {
            allocate_new_block();
        }
        temp = _block_cur_slot;
        _block_cur_slot = (Slot*)((char*)_block_cur_slot + _slot_size);
    }
    return temp;
}

void MemoryPool::deallocate(void* ptr) {
    if (!ptr) return;
    
    _free_list->push_front((Slot*) ptr);
}

void MemoryPool::allocate_new_block() {
    _block_list.push_front(operator new(_block_size));
    // printf("create new block.\n");

    _block_cur_slot = reinterpret_cast<Slot*>(_block_list.front());
    _block_end_slot = reinterpret_cast<Slot*>((char*)(_block_list.front()) + _block_size);
    
}

HashBucket::HashBucket() {
    for (int i = 0; i < MEMORY_POOL_NUM; ++i) {
        _pool[i] = new MemoryPool((i + 1) * SLOT_BASE_SIZE);
    }
}
HashBucket::~HashBucket() {
    for (int i = 0; i < MEMORY_POOL_NUM; ++i) {
        delete _pool[i];
        _pool[i] = nullptr;
    }
}

HashBucket& HashBucket::getInstance() {
    static HashBucket m;
    return m;
}

void* HashBucket::useMemory(size_t size, int t) {
    if (size <= 0) return nullptr;
    if (size > SLOT_MAX_SIZE)
        return operator new(size);
    
    // 分配大于size的最小Slot，大小为size / 8 向上取整
    return _pool[((size + 7) / SLOT_BASE_SIZE) - 1]->allocate(t);
}

void HashBucket::freeMemory(void* ptr, size_t size) {
    if (!ptr) return;
    if (size > SLOT_MAX_SIZE) {
        operator delete(ptr);
        return;
    }
    _pool[((size + 7) / SLOT_BASE_SIZE - 1)]->deallocate(ptr);
}

}
