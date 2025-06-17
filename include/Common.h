#pragma once

#include <cstddef>
#include <algorithm>

namespace mem {

#define MEMORY_POOL_NUM 64  // SLOT的大小有64种，大小为8-512（即64*8）
#define SLOT_BASE_SIZE 8  // 所有SLOT都必须是8个倍数
#define SLOT_MAX_SIZE 512  // SLOT最大尺寸

constexpr size_t ALIGNMENT = 8;  // 内存对齐
constexpr size_t MAX_BYTES = 256 * 1024;  // 内存块大小上限256kb
constexpr size_t FREE_LIST_SIZE = MAX_BYTES / ALIGNMENT;  // 缓存链表节点大小种类

constexpr size_t PAGE_SIZE = 4096;  // 页大小 4kb
constexpr size_t MIN_SPAN_PAGE_NUM = 8;  // 中心缓存单次获取的最小SPAN页数

constexpr size_t TC_MAX_LIST_SIZE = 64;  // ThreadCache中，每种大小的空闲链表存储的内存块数量上限

// 大小类管理
class SizeClass {
public:
    // 获得对齐后的内存大小（因为每个预分配的内存块都是内存对齐的）
    // 方法：向上取整到ALIGNMENT的最小倍数
    // return: bytes
    // 8: 1-8; 16: 9-16; 24: 17-24; ......
    static size_t get_alignedsize(size_t bytes) {
        return (bytes + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
    }

    // 获取内存块存储下标
    // 方法：对齐后，除以ALIGNMENT，再减一（散列压缩，原本是8,16,24,32,...，压缩后是0,1,2,3,...）
    // idx:size
    // 0: 1-8; 1: 9-16; 2: 17-24; ......
    static size_t get_index(size_t bytes) {
        bytes = std::max(bytes, ALIGNMENT);
        // 向上取整，再-1，得到下标
        return (bytes + ALIGNMENT - 1) / ALIGNMENT - 1;
    }

    // index转对齐后的内存块大小
    static size_t index_to_alignedsize(size_t index) {
        return (index + 1) * ALIGNMENT;
    }
};

}