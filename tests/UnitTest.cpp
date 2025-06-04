// 单元测试
// 测试方法：循环申请释放不同颗粒度的内存，测试其在不同颗粒度场景下的性能提升

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <chrono>
#include <future>
#include <random>
#include "./Sample.h"
#include "../include/MemoryPool.h"


TEST(functionalTest, baseCase) {
    P1* p1 = mem::newObj<P1>(4);
    ASSERT_EQ(p1->a, 4);
    mem::delObj<P1>(p1);

    P2* p2 = mem::newObj<P2>();
    ASSERT_NE(p2, nullptr);
    mem::delObj<P2>(p2);
}

// 边界检测，最小内存申请
TEST(functionalTest, minObj) {
    P0* p = mem::newObj<P0>();
    ASSERT_NE(p, nullptr);
    mem::delObj<P0>(p);
}

// 边界检测，最大内存申请
TEST(functionalTest, maxObj) {
    P5* p = mem::newObj<P5>();
    ASSERT_NE(p, nullptr);
    mem::delObj<P5>(p);
}

// 边界检测，超越最大内存申请，调用系统内存
TEST(functionalTest, overMaxObj) {
    P6* p = mem::newObj<P6>();
    ASSERT_NE(p, nullptr);
    mem::delObj<P6>(p);
}

// 检测内存复用是否正确
TEST(functionalTest, newAgain) {
    P1* p1 = mem::newObj<P1>(4);
    ASSERT_EQ(p1->a, 4);
    void* ptr = p1;
    mem::delObj<P1>(p1);

    P1* p2 = mem::newObj<P1>(3);
    ASSERT_EQ(p2->a, 3);

    ASSERT_EQ(ptr, p2);  // 复用了同一块内存

    mem::delObj<P1>(p2);
}

// 检测申请第二块block是否正常
TEST(functionalTest, newSecondBlock) {
    // 一个block的大小是4K，申请超过该大小的内存，检查是否正常
    std::vector<P5*> vec;
    for (int i = 0; i < 9; ++i) {
        vec.push_back(mem::newObj<P5>());
    }
    for (P5* p: vec) {
        ASSERT_NE(p, nullptr);
        mem::delObj(p);
    }
}

// 线程安全性检测
// 安全标准：1. 
TEST(threadSafeTest, base1) {
    auto func = [](int i){
        std::default_random_engine dre(i);
        std::uniform_int_distribution<int> id(10, 100);

        P1* p = mem::newObj<P1>(i);
        std::this_thread::sleep_for(std::chrono::milliseconds(id(dre)));
        ASSERT_EQ(p->a, i);
        mem::delObj<P1>(p);
    };
    for (int i = 0; i < 10; ++i)
        std::async(func, i);
    
}

int main (int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}