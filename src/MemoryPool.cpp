#include "../include/MemoryPool.h"

namespace mem {

MemoryPool::MemoryPool(size_t slot_size, size_t block_size): _block_size(block_size), _slot_size(slot_size) {
    _block_cur_slot = nullptr;
    _block_end_slot = nullptr;

    _free_list = nullptr;
}

MemoryPool::~MemoryPool() {
    for (auto& block: _block_list) {
        operator delete(block);
        block = nullptr;
    }
}

void* MemoryPool::allocate() {
    // 优先查询空闲链表
    Slot* slot = pop_free_list();
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
    
    push_free_list((Slot*)ptr);
}

// 头插法
void MemoryPool::push_free_list(Slot* slot) {
    std::lock_guard<std::mutex> lg(_mtx_for_free_list);
    if (!_free_list) {
        _free_list = slot;
        _free_list->next = nullptr;  // 一定要置空，否则会越界
        return;
    }

    slot->next = _free_list;
    _free_list = slot;
}
Slot* MemoryPool::pop_free_list() {
    std::lock_guard<std::mutex> lg(_mtx_for_free_list);
    Slot* old_head = _free_list;
        
    if (!_free_list) return nullptr;

    Slot* head = _free_list;
    _free_list = _free_list->next;
    return head;
}

void MemoryPool::allocate_new_block() {
    _block_list.push_front(operator new(_block_size));

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

void* HashBucket::useMemory(size_t size) {
    if (size <= 0) return nullptr;
    if (size > SLOT_MAX_SIZE)
        return operator new(size);
    
    // 分配大于size的最小Slot，大小为size / 8 向上取整
    return _pool[((size + 7) / SLOT_BASE_SIZE) - 1]->allocate();
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
