// 单元测试
// 测试方法：循环申请释放不同颗粒度的内存，测试其在不同颗粒度场景下的性能提升


#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include "../include/MemoryPool.h"

// 测试对象
struct P1 {
    P1() = default;
    P1(int b): a(b) {}
    int a;  // 4字节
};

struct P2 {
    int a[5];  // 20字节
};

struct P3 {
    int a[10];  // 40字节
};

struct P4 {
    int a[20];  // 80字节
};

// params: 单轮次申请释放次数，线程数，轮次
void Benchmark(size_t ntimes, size_t nworks, size_t rounds, const std::function<void()>& func) {
    std::vector<std::thread> vthread(nworks);  // 线程池
    size_t total_costtime = 0;  // 总耗时
    for (size_t i = 0; i < nworks; ++i) {
        vthread[i] = std::thread([&](){
            

            for (size_t j = 0; j < rounds; ++j) {
                size_t beg = clock();
                for (size_t k = 0; k < ntimes; ++k) {
                    func();
                }
                size_t end = clock();
                total_costtime += (end - beg);
            }

            
        });
    }
    for (auto& t: vthread) {
        t.join();
    }
    printf("%lu个线程并发执行%lu轮次，每轮次malloc&free %lu次，总计花费：%lu ms\n", nworks, rounds, ntimes, total_costtime);
}

// 功能测试
void functionalTest() {
    P1* p1 = mem::newObj<P1>(4);
    std::cout << p1->a << std::endl;
    mem::delObj<P1>(p1);

    P2* p2 = mem::newObj<P2>();
    mem::delObj<P2>(p2);
}


int main() {
    // 测试内存池

    std::cout << "===========================" << std::endl;
    std::cout << "===========================" << std::endl;
    // 测试new delete
    // Benchmark(100, 1, 100, [](){
    //     P1* p = new P1();
    //     delete p;
    //     P2* p2 = new P2();
    //     delete p2;
    //     P3* p3 = new P3();
    //     delete p3;
    //     P4* p4 = new P4();
    //     delete p4;
    // });

    functionalTest();

    return 0;
}