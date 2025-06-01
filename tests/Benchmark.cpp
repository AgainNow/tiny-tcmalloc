#include <benchmark/benchmark.h>
#include <thread>
#include <chrono>
#include <vector>
#include <array>
#include "./Sample.h"
#include "../include/MemoryPool.h"

int Add(int a, int b) {
    return a + b;
}

// params: 单轮次申请释放次数，线程数，轮次
void Benchmark(size_t ntimes, size_t nworks, size_t rounds, const std::function<void()>& func) {
    std::vector<std::thread> vthread(nworks);  // 线程池
    // size_t total_costtime = 0;  // 总耗时
    for (size_t i = 0; i < nworks; ++i) {
        vthread[i] = std::thread([&](){
            

            for (size_t j = 0; j < rounds; ++j) {
                for (size_t k = 0; k < ntimes; ++k) {
                    func();
                }
            }

            
        });
    }
    for (auto& t: vthread) {
        t.join();
    }
}


static void BM_New(benchmark::State& state) {
  for (auto _ : state)  // 由框架自动调整迭代次数，直到获得稳定的性能数据
    // Add(2, 3);
    Benchmark(100, 1, 100, [](){
        P1* p = new P1();
        delete p;
        P2* p2 = new P2();
        delete p2;
        P3* p3 = new P3();
        delete p3;
        P4* p4 = new P4();
        delete p4;
    });
}
BENCHMARK(BM_New);  // 注册测试函数

static void BM_MemPool(benchmark::State& state) {
  for (auto _ : state)
    // Add(2, 3);
    Benchmark(100, 1, 100, [](){
        P1* p = mem::newObj<P1>();
        mem::delObj<P1>(p);
        P2* p2 = mem::newObj<P2>();
        mem::delObj<P2>(p2);
        P3* p3 = mem::newObj<P3>();
        mem::delObj<P3>(p3);
        P4* p4 = mem::newObj<P4>();
        mem::delObj<P4>(p4);
    });
}
BENCHMARK(BM_MemPool);

// 单线程 重复申请释放 P1
static void BM_MemSingleP1(benchmark::State& state) {
    const size_t n = state.range(0);
    std::vector<P1*> vec;
    vec.resize(n);
    for (auto _: state) {
        // 申请n次，再释放n次
        for (int i = 0; i < n; ++i) {
            vec[i] = mem::newObj<P1>(); 
        }
        for (int i = 0; i < n; ++i) {
            mem::delObj<P1>(vec[i]);
        }
    }
}
BENCHMARK(BM_MemSingleP1)->Arg(1000);

// 单线程 重复申请释放 P1
static void BM_NewSingleP1(benchmark::State& state) {
    const size_t n = state.range(0);
    std::vector<P1*> vec;
    vec.resize(n);
    for (auto _: state) {
        // 申请n次，再释放n次
        for (int i = 0; i < n; ++i) {
            vec[i] = new P1; 
        }
        for (int i = 0; i < n; ++i) {
            delete vec[i];
        }
    }
}
BENCHMARK(BM_NewSingleP1)->Arg(1000);

BENCHMARK_MAIN();