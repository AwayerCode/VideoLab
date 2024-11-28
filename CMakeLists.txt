cmake_minimum_required(VERSION 3.10)
project(my_project VERSION 1.0)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加 src 子目录
add_subdirectory(src) 