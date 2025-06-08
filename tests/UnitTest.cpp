// 单元测试
// 测试方法：循环申请释放不同颗粒度的内存，测试其在不同颗粒度场景下的性能提升

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <chrono>
#include <future>
#include <random>
#include <cstring>
#include "./Sample.h"
#include "../include/MemoryPool.h"
#include "../include/LockFreeList.h"

using namespace mem;

// // 大小测试
// TEST(functionalTest, ObjectSizeTest) {
//     // 最小值
//     {
//         auto p = newObj<Sample<0>>();
//         ASSERT_EQ(p, nullptr);
//         delObj(p);
//     }

//     {
//         auto p = newObj<Sample<1>>();
//         ASSERT_NE(p, nullptr);
//         delObj(p);
//     }

//     {
//         auto p = newObj<Sample<7>>();
//         ASSERT_NE(p, nullptr);
//         delObj(p);
//     }
//     {
//         auto p = newObj<Sample<8>>();
//         ASSERT_NE(p, nullptr);
//         delObj(p);
//     }
//     {
//         auto p = newObj<Sample<9>>();
//         ASSERT_NE(p, nullptr);
//         delObj(p);
//     }

//     {
//         auto p = newObj<Sample<16>>();
//         ASSERT_NE(p, nullptr);
//         delObj(p);
//     }
//     {
//         // 最大值
//         auto p = newObj<Sample<512>>();
//         ASSERT_NE(p, nullptr);
//         delObj(p);
//     }
//     {
//         // 超过最大值后使用new
//         auto p = newObj<Sample<513>>();
//         ASSERT_NE(p, nullptr);
//         delObj(p);
//     }
// }

// // 数量检测
// TEST(functionalTest, countTest) {
//     // 单次申请的内存数量上限，单位为8Byte
//     std::vector<int> ns = {1, 2, 512, 513, 512 * 16};  // 测试单次最大申请64Mb内存
//     std::vector<Sample<8>*> vec{};
//     for (auto n: ns) {
//         for (int i = 0; i < n; i++) {
//             auto p = newObj<Sample<8>>();
//             ASSERT_NE(p, nullptr);
//             vec.push_back(p);
//         }
//         for (auto p: vec) {
//             delObj(p);
//         }
//         vec.clear();
//     }
// }

// // 一块Block切分是否正常，示例必须是其他测试函数没有用过的池
// TEST(functionalTest, blockTest) {
//     // 一个Block可以申请8个256byte的对象
//     const int slot_size = 256;
//     int count = 4096 / slot_size;
//     std::vector<Sample<slot_size>*> vec{};
//     for (int i = 0; i < count; ++i) {
//         auto p = newObj<Sample<slot_size>>();
//         ASSERT_NE(p, nullptr);
//         vec.push_back(p);
//     }
//     ASSERT_EQ((char*)(vec[1]) - (char*)(vec[0]), slot_size);
//     ASSERT_EQ((char*)(vec[2]) - (char*)(vec[1]), slot_size);
//     ASSERT_EQ((char*)(vec[3]) - (char*)(vec[2]), slot_size);
//     for (auto p: vec) {
//         delObj(p);
//     }
//     vec.clear();
// }

// // 检测内存复用是否正确
// TEST(functionalTest, memMultiplexing) {
//     auto p1 = newObj<Sample<4>>();
//     ASSERT_NE(p1, nullptr);
//     void* ptr = p1;
//     delObj<Sample<4>>(p1);

//     auto p2 = newObj<Sample<4>>();
//     ASSERT_NE(p2, nullptr);

//     ASSERT_EQ(ptr, p2);  // 复用了同一块内存

//     delObj<Sample<4>>(p2);
// }


// // 线程安全检测
// TEST(MultiThreadTest, threadSafeTest) {
//     auto func = [](int i){
//         // std::cout << i << ": " << std::this_thread::get_id() << std::endl;
//         std::default_random_engine dre(i);
//         std::uniform_int_distribution<int> id(10, 100);
//         std::thread::id tid = std::this_thread::get_id();

//         // 可以排查ABA问题
//         std::vector<Sample<16>*> vec{};
//         for (int k = 0; k < 8; k++) {
//             for (int j = 0; j < 4; j++) {
//                 auto p = newObj<Sample<16>>();
//                 memcpy(p, &tid, sizeof(tid));
//                 vec.push_back(p);
//                 std::this_thread::sleep_for(std::chrono::milliseconds(id(dre)));
//                 ASSERT_EQ(*(std::thread::id*)(p->dat), tid);
//             }
//             for (auto p: vec) {
//                 delObj<Sample<16>>(p);
//             }
//             vec.clear();
//         }
//     };
//     std::vector<std::future<void>> futures;
//     for (int i = 0; i < 8; ++i)
//         futures.push_back(std::async(std::launch::async, func, i));
//     for (auto& f : futures) {
//         f.get();
//     }
    
// }

// // ABA问题验证
// TEST(LockFreeListTest, existsABA) {
//     auto list = new ABALockFreeList();
//     list->push_front(new Slot());  // 节点1
//     list->push_front(new Slot());  // 节点2
//     list->print();

//     // 目标是
//     auto func1 = [&](){
//         auto p = list->pop_front(100);
//         printf("[T1] pop p = %p\n", p);
//     };
//     // Slot* p2 = nullptr;
//     auto func2 = [&](){
//         std::this_thread::sleep_for(std::chrono::milliseconds(20));
//         auto p1 = list->pop_front();
//         printf("[T2] pop p1 = %p\n", p1);
//         auto p2 = new Slot();
//         list->push_front(p2);  // 节点3
//         printf("[T2] push p2 = %p\n", p2);  // 触发ABA问题时，p2会丢失
//         list->push_front(p1);  // 节点4
//         printf("[T2] push p1 = %p\n", p1);
//     };

//     auto res1 = std::async(std::launch::async, func1);
//     auto res2 = std::async(std::launch::async, func2);
//     res1.get();
//     res2.get();

//     ASSERT_EQ(1, list->size());
//     list->print();
// }

// // ABA问题验证
// TEST(LockFreeListTest, NotExistsABA) {
//     auto list = new LockFreeList();
//     list->push_front(new Slot());  // 节点1
//     list->push_front(new Slot());  // 节点2
//     list->print();

//     auto func1 = [&](){
//         auto p = list->pop_front(100);
//         printf("[T1] pop p = %p\n", p);
//     };

//     auto func2 = [&](){
//         std::this_thread::sleep_for(std::chrono::milliseconds(20));
//         auto p1 = list->pop_front();
//         printf("[T2] pop p1 = %p\n", p1);
//         auto p2 = new Slot();
//         list->push_front(p2);  // 节点3
//         printf("[T2] push p2 = %p\n", p2);  // 触发ABA问题时，p2会丢失
//         list->push_front(p1);  // 节点4
//         printf("[T2] push p1 = %p\n", p1);
//     };

//     auto res1 = std::async(std::launch::async, func1);
//     auto res2 = std::async(std::launch::async, func2);
//     res1.get();
//     res2.get();

//     ASSERT_EQ(2, list->size());
//     list->print();
// }

// 单轮次申请释放次数 线程数 轮次
size_t BenchmarkMemoryPool(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks); // 线程池
	size_t total_costtime = 0;
	for (size_t k = 0; k < nworks; ++k) // 创建 nworks 个线程
	{
		vthread[k] = std::thread([&]() {
			for (size_t j = 0; j < rounds; ++j)
			{
				size_t begin1 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
                    auto p1 = newObj<Sample<4>>(); // 内存池对外接口
                    delObj<Sample<4>>(p1);
                    auto p2 = newObj<Sample<20>>();
                    delObj<Sample<20>>(p2);
                    auto p3 = newObj<Sample<40>>();
                    delObj<Sample<40>>(p3);
                    auto p4 = newObj<Sample<80>>();
                    delObj<Sample<80>>(p4);
				}
				size_t end1 = clock();

				total_costtime += end1 - begin1;
			}
		});
	}
	for (auto& t : vthread)
	{
		t.join();
	}
	printf("%lu个线程并发执行%lu轮次，每轮次newElement&deleteElement %lu次，总计花费：%lu ms\n", nworks, rounds, ntimes, total_costtime);
    return total_costtime;
}

size_t BenchmarkNew(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks);
	size_t total_costtime = 0;
	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = std::thread([&]() {
			for (size_t j = 0; j < rounds; ++j)
			{
				size_t begin1 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
                    auto p1 = new Sample<4>;
                    delete p1;
                    auto p2 = new Sample<20>;
                    delete p2;
                    auto p3 = new Sample<40>;
                    delete p3;
                    auto p4 = new Sample<80>;
                    delete p4;
				}
				size_t end1 = clock();
				
				total_costtime += end1 - begin1;
			}
		});
	}
	for (auto& t : vthread)
	{
		t.join();
	}
	printf("%lu个线程并发执行%lu轮次，每轮次malloc&free %lu次，总计花费：%lu ms\n", nworks, rounds, ntimes, total_costtime);
    return total_costtime;
}

TEST(perfTest, base) {
    size_t t1 = BenchmarkMemoryPool(100, 1, 10);
    size_t t2 = BenchmarkNew(100, 1, 10);
    ASSERT_GE(t2, t1);
}

int main (int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}