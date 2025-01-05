cmake_minimum_required(VERSION 3.10)

project(CrossPlatform)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt设置
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 添加子目录
add_subdirectory(src/ffmpeg)
add_subdirectory(src/ui)

# 查找Qt包
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# 查找Boost库
find_package(Boost REQUIRED COMPONENTS system thread context)

# 收集源文件
set(SOURCES
    src/main.cpp
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接库
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        cp_ffmpeg
        cp_ui
        Qt6::Core
        Qt6::Widgets
        Boost::system
        Boost::thread
        Boost::context
) 