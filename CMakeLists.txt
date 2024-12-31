cmake_minimum_required(VERSION 3.10)
project(CrossPlatform VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt settings
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find packages
find_package(Boost REQUIRED COMPONENTS 
    context 
    system      # ASIO 需要
    thread      # ASIO 需要
)

# 添加 FFmpeg 子项目
add_subdirectory(src/ffmpeg)

# Collect source files
set(SOURCES
    src/main.cpp
)

set(INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/patterns
    ${PROJECT_SOURCE_DIR}/src/modern
    ${PROJECT_SOURCE_DIR}/src/threads
    ${PROJECT_SOURCE_DIR}/src/io
    ${PROJECT_SOURCE_DIR}/src/utils
    ${PROJECT_SOURCE_DIR}/src/template
    ${PROJECT_SOURCE_DIR}/src/ffmpeg
)

# 添加可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接依赖库
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Boost::context
    Boost::system
    Boost::thread
    cp_ffmpeg
)

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS}) 