#include "PageCache.h"
#include "Common.h"

#include <sys/mman.h>
#include <iostream>

namespace mem {

PageCache::PageCache(size_t slot_size, size_t block_size) {
    // _element_size = slot_size;
    // _size = block_size;

    // _cur_slot = nullptr;
    // _end_slot = nullptr;
}

PageCache::~PageCache() {
    // for (auto& block: _free_list) {
    //     operator delete(block);
    //     block = nullptr;
    // }
}



void* PageCache::allocate_span(size_t page_num) {
    std::lock_guard<std::mutex> lg(_mtx);
    // 查找合适的空闲span
    // lower_bound返回第一个key大于等于page_num的元素的迭代器
    auto it = _free_span.lower_bound(page_num);
    if (it != _free_span.end()) {
        Span* span = it->second;

        // 移除记录
        if (span->next) {
            _free_span[it->first] = span->next;  // 头删法
        } else {
            _free_span.erase(it);
        }

        // 如果span大于需要的page_num则进行分割
        if (span->page_num > page_num) {
            // 超出部分（尾部）
            Span* new_span = new Span;
            new_span->page_addr = (char*)(span->page_addr) + page_num * PAGE_SIZE;
            new_span->page_num = span->page_num - page_num;
            new_span = nullptr;

            // 存入空闲链表，并更新_all_span_record中的记录
            auto& list = _free_span[new_span->page_num];
            new_span->next = list;
            list = new_span;
            _all_span_record[new_span->page_addr] = new_span;  // TODO 待验证

            // 更新span大小
            span->page_num = page_num;
        }
        // 记录span用于回收
        _all_span_record[span->page_addr] = span;
        return span->page_addr;

    } else {  // 没找到，则向系统申请
        void* ptr = system_alloc(page_num);
        if (!ptr) return nullptr;

        // 记录span用于回收
        Span* span = new Span;
        span->page_addr = ptr;
        span->page_num = page_num;
        span->next = nullptr;
        _all_span_record[ptr] = span;

        return ptr;
    }
}

void PageCache::deallocate_span(void* ptr, size_t page_num) {
    std::lock_guard<std::mutex> lg(_mtx);
    
    // 所有page申请的内存都会有记录
    auto it = _all_span_record.find(ptr);
    if (it == _all_span_record.end()) return;

    Span* span = it->second;

    // 尝试合并相邻span（减少外部碎片）
    // 1、查看要插入的span后一位地址是否在记录中，即是否有对应的next_span
    void* next_addr = (char*)ptr + page_num * PAGE_SIZE;
    auto next_it = _all_span_record.find(next_addr);
    if (next_it != _all_span_record.end()) {
        Span* next_span = next_it->second;  // 目标相邻节点

        // 2、检查next_span是否空闲
        bool found = false;
        auto& next_list_head = _free_span[next_span->page_num];
        // 3. 若是空闲节点，检查是否是头结点
        if (next_list_head == next_span->next) {
            next_list_head = next_span->next;  // 若是，将空闲链节点的起始地址指针直接指向插入的节点
            found = true;
        } else if (next_list_head) {
            // 4. 若不是头结点，遍历链表，找到next_span并删除
            Span* prev_span = next_list_head;
            while (prev_span) {
                if (prev_span->next == next_span) {
                    prev_span->next = next_span->next;  // 删除next_span节点
                    found = true;
                    break;
                }
                prev_span = prev_span->next;
            }
        }

        // 合并span其他属性,next_span合入当前span
        if (found) {
            span->page_num += next_span->page_num;
            _all_span_record.erase(next_addr);
            delete next_span;
        }
    }

    // 头插法插入空闲列表
    auto& list = _free_span[span->page_num];
    span->next = list;
    list = span;

}

// void* PageCache::allocate() {
//     // 互斥锁
//     // {  // TODO 调整互斥锁的范围
//         // std::lock_guard<std::mutex> lg(_mtx);
//         if (_cur_slot >= _end_slot) {
//             allocate_operator_new();
//             // allocate_mmap();
//             // printf("allocate_operator_new() %ld\n", _element_size);
//         }
//     // } 
//     Slot* slot = _cur_slot;
//     // slot->next = nullptr;
//     _cur_slot = (Slot*)((char*)_cur_slot + _element_size);
//     return slot;
// }

// void* PageCache::allocate_block() {
//     // 互斥锁
//     std::lock_guard<std::mutex> lg(_mtx);
//     return operator new(_size);
// }

// void PageCache::allocate_operator_new() {
//     _free_list.push_front(operator new(_size));

//     _cur_slot = (Slot*)(_free_list.front());
//     _end_slot = (Slot*)((char*)(_free_list.front()) + _size);
// }

void* PageCache::system_alloc(size_t page_num) {
    size_t size = page_num * PAGE_SIZE;
    // 使用mmap分配内存
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) return nullptr;

    // 清零内存
    memset(ptr, 0, size);
    return ptr;
}

// void PageCache::allocate_mmap() {
//     void* ptr = system_alloc(1);  // 申请1页

//     _free_list.push_front(ptr);

//     _cur_slot = (Slot*)(_free_list.front());
//     _end_slot = (Slot*)((char*)(_free_list.front()) + _size);
// }

}