cmake_minimum_required(VERSION 3.20)
project(Orderbook CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Release build flags (for benchmarking)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

add_executable(orderbook
    src/main.cpp
    src/Order.cpp
    src/Orderbook.cpp
)
target_include_directories(orderbook PRIVATE include)

# add_executable(benchmark
#     benchmark/benchmark.cpp
#     src/Order.cpp
#     src/Orderbook.cpp
# )
# target_include_directories(benchmark PRIVATE include)
