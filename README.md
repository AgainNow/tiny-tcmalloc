# 高并发内存池

## 目的

- 参考卡码网的MemoryPool
- 参考的tcmalloc

## 用法

```C++
#include "MemoryPool.h"
// Example1
size_t size = 8;
void* ptr = mem::MemoryPool::get_instance()::allocate(size);
mem::MemoryPool::get_instance()::deallcoate(ptr, size);

// Example2
Object* obj = newObj<Object>(params...);
delObj<Object>(obj);
```

## 测试报告

功能测试：
Starting memory pool tests...
Running basic allocation test...
Basic allocation test passed!
Running memory writing test...
Memory writing test passed!
Running multi-threading test...
Multi-threading test passed!
Running edge cases test...
Edge cases test passed!
Running stress test...
Stress test passed!
All tests passed successfully!

性能测试：
Starting performance tests...
Warming up memory systems...
Warmup complete.


Testing small allocations (100000 allocations of 32 bytes):
Memory Pool: 4.088 ms
New/Delete: 14.468 ms

Testing multi-threaded allocations (4 threads, 25000 allocations each):
Memory Pool: 5.222 ms
New/Delete: 18.166 ms

Testing mixed size allocations (50000 allocations):
Memory Pool: 14.885 ms
New/Delete: 13.487 ms

## 测试标准

1. 功能测试
    - 单线程（正常流程、边界检测）
        - 申请各种大小的对象（参考Slot大小）
            0byte，1byte
            7byte，8byte，9byte
            16byte，32byte，64byte，128byte，256byte，512byte
            513byte
        - 申请各种数量的对象（参考Block大小，以8byte为单位）
            1个block：1个，2个，512个
            2个block：
            4个block：
            上限：512个block
        - 验证1个Block可以申请的对象数量
        - 内存复用检测
    - 多线程
        - 线程安全
            - ABA问题
2. 性能测试
    综合性能必须跑赢new/delete，测试出优势的地方，输出性能报告

