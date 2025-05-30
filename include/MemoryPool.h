/**
 * 内存池 v1.0
 * 线程不安全，不考虑内存对齐
 * 使用方式：
 *     Object* obj = newObj(Object, param1, param2, ...);
 *     deleteObj(obj);
 * 
 */
#pragma once

#include <iostream>

namespace mem {

#define MEMORY_POOL_NUM 64  // SLOT的大小有64中，大小为8-512（即64*8）
#define SLOT_BASE_SIZE 8  // 所有SLOT都必须是8个倍数
#define SLOT_MAX_SIZE 512  // SLOT最大尺寸

// 内存槽
struct Slot {
    Slot(): next(nullptr) {}
    Slot* next;
};

// 只存放同一大小的对象
class MemoryPool {
public:
    MemoryPool(size_t slot_size, size_t block_size = 4096);
    ~MemoryPool();
public:
    // 申请内存
    void* allocate();
    // 释放内存
    void deallocate(void* ptr);
private:
    // 空间链表的插入和获取
    void push_free_list(Slot* slot);
    Slot* pop_free_list();

    // 申请新的内存块
    void allocate_new_block();
private:
    size_t _slot_size;  // 内存块分割成内存槽的大小

    void* _block;  // 向系统申请的内存块
    size_t _block_size;  // 向系统申请的内存块的大小
    Slot* _block_cur_slot;  // 块上下一个可用内存槽
    Slot* _block_end_slot;  // 块上最后一块可用内存槽
    
    Slot* _free_list;  // 空闲内存槽链表

};


class HashBucket {
public:
    HashBucket();
    ~HashBucket();
public:
    static HashBucket& getMemoryPool();
public:
    // 申请大小为size的内存块
    void* useMemory(size_t size);
    // 释放内存块
    void freeMemory(void* ptr, size_t size);
private:
    MemoryPool* _pool[MEMORY_POOL_NUM];
};

// 使用: 
//     Obj* p = newObj(Obj, parm1, parm2, ...);
//     delObj(p);
template<typename T, typename... Args>
T* newObj(Args&&... args) {
    T* p = nullptr;
    // 申请内存 TODO
    p = (T*)HashBucket::getMemoryPool().useMemory(sizeof(T));
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
        // 回收内存  TODO
        HashBucket::getMemoryPool().freeMemory((void*)p, sizeof(T));
    }
}

}