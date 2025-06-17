# CMake minimum version
cmake_minimum_required(VERSION 3.10)

# Project name
project(MemPool)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-Wall -O2)

# Include directories
include_directories(include)

# Source files
file(GLOB SRC_FILES "src/*.cpp")
file(GLOB TEST_FILES "tests/*.cpp")

# 查找 Google Test 包
find_package(GTest REQUIRED)
find_package(benchmark REQUIRED)

# Add the executable for the main target
# add_executable(${PROJECT_NAME} ${SRC_FILES} ${TEST_FILES})
add_executable(${PROJECT_NAME} ${SRC_FILES} "tests/UnitTest.cpp")

# Add compile options
target_compile_options(${PROJECT_NAME} PRIVATE -g -pthread)

# Link libraries
target_link_libraries(${PROJECT_NAME} pthread GTest::GTest GTest::Main)


add_executable(${PROJECT_NAME}_Benchmark ${SRC_FILES} "tests/Benchmark.cpp")
target_link_libraries(${PROJECT_NAME}_Benchmark PRIVATE benchmark::benchmark pthread)


add_executable(${PROJECT_NAME}_CustomBenchmark ${SRC_FILES} "tests/PerformanceTest.cpp")
target_link_libraries(${PROJECT_NAME}_CustomBenchmark PRIVATE pthread)

# 添加测试
enable_testing()
add_test(NAME MemUnitTest COMMAND ${PROJECT_NAME})

# Link libraries (if any)
# target_link_libraries(${PROJECT_NAME} <library_name>)