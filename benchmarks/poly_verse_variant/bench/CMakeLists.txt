cmake_minimum_required(VERSION 3.5)

project(bench LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(benchmark REQUIRED)

add_executable(bench main.cpp)

target_link_libraries(bench PRIVATE Threads::Threads benchmark::benchmark -larmadillo)
set (CMAKE_CXX_FLAGS_RELEASE " -O3 -march=native  -funroll-loops -DNDEBUG ")
