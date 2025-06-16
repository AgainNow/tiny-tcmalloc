#include <benchmark/benchmark.h>
#include <thread>
#include <chrono>
#include <vector>
#include <array>
#include <future>
#include "./Sample.h"
#include "../include/MemoryPool.h"

using namespace mem;

// 预热内存池
void WarmUp() {
    std::vector<std::pair<void*, size_t>> warmupPtrs;
    // std::vector<size_t> sizes = {8, 16, 32, 64};
    std::vector<size_t> sizes = {32};
    
    // 预热内存池
    for (int i = 0; i < 64; ++i) {
        for (size_t size : sizes) {
            void* p = MemoryPool::allocate(size);
            warmupPtrs.emplace_back(p, size);  // 存储指针和对应的大小
        }
    }
    
    // 释放预热内存
    for (auto& x : warmupPtrs) {
        MemoryPool::deallocate(x.first, x.second);  // 使用实际分配的大小进行释放
    }
}

// 单线程，单一大小对象，申请后延迟释放，循环申请
static void BM_1ThreadRelDelay(benchmark::State& state) {
    const size_t rounds = state.range(0);  // 循环次数
    const size_t counts = state.range(1);  // 单次循环申请的对象数量
    WarmUp();
    std::vector<Sample<32>*> vec;
    vec.resize(counts);
    for (auto _: state) {
        for (int i = 0; i < rounds; ++i) {
            for (int j = 0; j < counts; ++j) {
                vec[j] = newObj<Sample<32>>();
            }
            for (auto p: vec) {
                delObj<Sample<32>>(p);
            }
        }
    }
}
// BENCHMARK(BM_1ThreadRelDelay)->Args({1, 64})->Threads(1);

// 单线程，单一大小对象，申请后延迟释放，循环申请
static void BM_1ThreadRelDelayNew(benchmark::State& state) {
    const size_t rounds = state.range(0);  // 循环次数
    const size_t counts = state.range(1);  // 单次循环申请的对象数量
    std::vector<Sample<32>*> vec;
    vec.resize(counts);
    for (auto _: state) {
        for (int i = 0; i < rounds; ++i) {
            for (int j = 0; j < counts; ++j) {
                vec[j] = new Sample<32>();
                // void* p = malloc(8);
                // vec.emplace_back(new(p) Sample<8>());
            }
            for (auto p: vec) {
                delete p;
                // free(p);
            }
        }
    }
}
// BENCHMARK(BM_1ThreadRelDelayNew)->Args({1, 64})->Threads(1);

// 单线程，多种大小对象，申请就释放，循环申请
static void BM_1ThreadRelQuickly(benchmark::State& state) {
    for (auto _: state) {
        for (int i = 0; i < 64; ++i) {  // 循环申请64次
            auto p1 = newObj<Sample<8>>();
            delObj<Sample<8>>(p1);
            // auto p2 = newObj<Sample<16>>();
            // delObj<Sample<16>>(p2);
            // auto p3 = newObj<Sample<32>>();
            // delObj<Sample<32>>(p3);
            // auto p4 = newObj<Sample<64>>();
            // delObj<Sample<64>>(p4);
        }
    }
}
// BENCHMARK(BM_1ThreadRelQuickly)->Threads(1);

// 单线程，多种大小对象，申请就释放，循环申请
static void BM_1ThreadRelQuicklyNew(benchmark::State& state) {
    for (auto _: state) {
        for (int i = 0; i < 64; ++i) {
            auto p1 = new Sample<8>();
            delete p1;
            // auto p2 = new Sample<16>();
            // delete p2;
            // auto p3 = new Sample<32>();
            // delete p3;
            // auto p4 = new Sample<64>();
            // delete p4;
        }
    }
}
// BENCHMARK(BM_1ThreadRelQuicklyNew)->Threads(1);

// 多线程，单一大小对象，申请后延迟释放，循环申请
static void BM_NThreadRelDealy(benchmark::State& state) {
    for (auto _: state) {
        auto func = [](){
            std::vector<Sample<8>*> vec;
            for (int i = 0; i < 1; ++i) {
                for (int j = 0; j < 64; ++j) { 
                    vec.emplace_back(newObj<Sample<8>>());
                }
                for (auto p: vec) {
                    delObj<Sample<8>>(p);
                }
                vec.clear();
            }
        };

        std::vector<std::future<void>> vec;
        for (int i = 0; i < 4; ++i) {
            vec.push_back(std::async(std::launch::async, func));
        }
        for (auto& f: vec) {
            f.get();
        }
    }
}
BENCHMARK(BM_NThreadRelDealy)->Threads(1);

// 多线程，单一大小对象，申请后延迟释放，循环申请
static void BM_NThreadRelDealyNew(benchmark::State& state) {
    for (auto _: state) {
        auto func = [](){
            std::vector<Sample<8>*> vec;
            for (int i = 0; i < 1; ++i) {
                for (int j = 0; j < 64; ++j) {
                    vec.emplace_back(new Sample<8>());
                }
                for (auto p: vec) {
                    delete p;
                }
                vec.clear();
            }
        };

        std::vector<std::future<void>> vec;
        for (int i = 0; i < 4; ++i) {
            vec.push_back(std::async(std::launch::async, func));
        }
        for (auto& f: vec) {
            f.get();
        }
    }
}
BENCHMARK(BM_NThreadRelDealyNew)->Threads(1);

// 多线程，多种大小对象，申请立即释放，循环申请；ABA问题会有明显段错误
static void BM_NThreadRelQuickly(benchmark::State& state) {
    for (auto _: state) {
        auto func = [](){
            for (int i = 0; i < 16; ++i) {
                auto p1 = newObj<Sample<8>>();
                delObj<Sample<8>>(p1);
                auto p2 = newObj<Sample<16>>();
                delObj<Sample<16>>(p2);
                auto p3 = newObj<Sample<32>>();
                delObj<Sample<32>>(p3);
                auto p4 = newObj<Sample<64>>();
                delObj<Sample<64>>(p4);
            }
        };

        std::vector<std::future<void>> vec;
        for (int i = 0; i < 4; ++i) {
            vec.push_back(std::async(std::launch::async, func));
        }
        for (auto& f: vec) {
            f.get();
        }
    }
}
// BENCHMARK(BM_NThreadRelQuickly)->Threads(1);

// 多线程，多种大小对象，申请立即释放，循环申请；ABA问题会有明显段错误
static void BM_NThreadRelQuicklyNew(benchmark::State& state) {
    for (auto _: state) {
        auto func = [](){
            for (int i = 0; i < 16; ++i) {
                auto p1 = new Sample<8>();
                delete p1;
                auto p2 = new Sample<16>();
                delete p2;
                auto p3 = new Sample<32>();
                delete p3;
                auto p4 = new Sample<64>();
                delete p4;
            }
        };

        std::vector<std::future<void>> vec;
        for (int i = 0; i < 4; ++i) {
            vec.push_back(std::async(std::launch::async, func));
        }
        for (auto& f: vec) {
            f.get();
        }
    }
}
// BENCHMARK(BM_NThreadRelQuicklyNew)->Threads(1);

BENCHMARK_MAIN();
