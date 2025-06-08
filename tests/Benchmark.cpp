#include <benchmark/benchmark.h>
#include <thread>
#include <chrono>
#include <vector>
#include <array>
#include <future>
#include "./Sample.h"
#include "../include/MemoryPool.h"

using namespace mem;

// 单线程，单一大小对象，申请后延迟释放，循环申请
static void BM_1ThreadRelDelay(benchmark::State& state) {
    const size_t rounds = state.range(0);  // 循环次数
    const size_t counts = state.range(1);  // 单次循环申请的对象数量
    std::vector<Sample<8>*> vec;
    for (auto _: state) {
        for (int i = 0; i < rounds; ++i) {  // 循环申请64次
            for (int j = 0; j < counts; ++j) {
                vec.emplace_back(newObj<Sample<8>>());
            }
            for (auto p: vec) {
                delObj<Sample<8>>(p);
            }
            vec.clear();
        }
    }
}
BENCHMARK(BM_1ThreadRelDelay)->Args({64, 128})->Args({128, 64});

// 单线程，单一大小对象，申请后延迟释放，循环申请
static void BM_1ThreadRelDelayNew(benchmark::State& state) {
    const size_t rounds = state.range(0);  // 循环次数
    const size_t counts = state.range(1);  // 单次循环申请的对象数量
    std::vector<Sample<8>*> vec;
    for (auto _: state) {
        for (int i = 0; i < rounds; ++i) {
            for (int j = 0; j < counts; ++j) {
                vec.emplace_back(new Sample<8>());
                // void* p = malloc(8);
                // vec.emplace_back(new(p) Sample<8>());
            }
            for (auto p: vec) {
                delete p;
                // free(p);
            }
            vec.clear();
        }
    }
}
BENCHMARK(BM_1ThreadRelDelayNew)->Args({64, 128})->Args({128, 64});

// 单线程，多种大小对象，申请就释放，循环申请
static void BM_1ThreadRelQuickly(benchmark::State& state) {
    for (auto _: state) {
        for (int i = 0; i < 1024; ++i) {  // 循环申请64次
            auto p1 = newObj<Sample<4>>();
            delObj<Sample<4>>(p1);
            auto p2 = newObj<Sample<20>>();
            delObj<Sample<20>>(p2);
            auto p3 = newObj<Sample<40>>();
            delObj<Sample<40>>(p3);
            auto p4 = newObj<Sample<80>>();
            delObj<Sample<80>>(p4);
        }
    }
}
BENCHMARK(BM_1ThreadRelQuickly);

// 单线程，多种大小对象，申请就释放，循环申请
static void BM_1ThreadRelQuicklyNew(benchmark::State& state) {
    for (auto _: state) {
        for (int i = 0; i < 1024; ++i) {
            auto p1 = new Sample<4>();
            delete p1;
            auto p2 = new Sample<20>();
            delete p2;
            auto p3 = new Sample<40>();
            delete p3;
            auto p4 = new Sample<80>();
            delete p4;
        }
    }
}
BENCHMARK(BM_1ThreadRelQuicklyNew);

// 多线程，单一大小对象，申请后延迟释放，循环申请
static void BM_NThreadRelDealy(benchmark::State& state) {
    for (auto _: state) {
        auto func = [](){
            std::vector<Sample<8>*> vec;
            for (int i = 0; i < 16; ++i) {
                for (int j = 0; j < 8; ++j) { 
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
BENCHMARK(BM_NThreadRelDealy);

// 多线程，单一大小对象，申请后延迟释放，循环申请
static void BM_NThreadRelDealyNew(benchmark::State& state) {
    for (auto _: state) {
        auto func = [](){
            std::vector<Sample<8>*> vec;
            for (int i = 0; i < 16; ++i) {
                for (int j = 0; j < 8; ++j) {
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
BENCHMARK(BM_NThreadRelDealyNew);

// 多线程，多种大小对象，申请立即释放，循环申请；ABA问题会有明显段错误
static void BM_NThreadRelQuickly(benchmark::State& state) {
    for (auto _: state) {
        auto func = [](){
            for (int i = 0; i < 16; ++i) {
                auto p1 = newObj<Sample<4>>();
                delObj<Sample<4>>(p1);
                auto p2 = newObj<Sample<20>>();
                delObj<Sample<20>>(p2);
                auto p3 = newObj<Sample<40>>();
                delObj<Sample<40>>(p3);
                auto p4 = newObj<Sample<80>>();
                delObj<Sample<80>>(p4);
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
BENCHMARK(BM_NThreadRelQuickly);

// 多线程，多种大小对象，申请立即释放，循环申请；ABA问题会有明显段错误
static void BM_NThreadRelQuicklyNew(benchmark::State& state) {
    for (auto _: state) {
        auto func = [](){
            for (int i = 0; i < 16; ++i) {
                auto p1 = new Sample<4>();
                delete p1;
                auto p2 = new Sample<20>();
                delete p2;
                auto p3 = new Sample<40>();
                delete p3;
                auto p4 = new Sample<80>();
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
BENCHMARK(BM_NThreadRelQuicklyNew);

BENCHMARK_MAIN();