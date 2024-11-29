cmake_minimum_required(VERSION 3.10)
project(Console VERSION 1.0)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 收集源文件
set(SOURCES
    src/main.cpp
    src/FileInfo/FileInfo.h
    src/FileInfo/FileInfo.cpp
    src/MemMonitor/MemMonitor.h
    src/MemMonitor/MemMonitor.cpp
    src/GlobalRoute.h
    src/GlobalRoute.cpp
)

# 收集头文件目录
set(INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/FileInfo
    ${PROJECT_SOURCE_DIR}/src/MemMonitor
)

# 添加可执行文件，同时包含源文件和头文件
add_executable(Console ${SOURCES})

# 添加头文件目录
target_include_directories(Console
    PRIVATE
        ${INCLUDE_DIRS}
) 