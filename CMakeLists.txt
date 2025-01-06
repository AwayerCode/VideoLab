cmake_minimum_required(VERSION 3.16)

project(CrossPlatform)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置自动生成的文件输出目录
set(CMAKE_AUTOMOC_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/automoc)
set(CMAKE_AUTORCC_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/autorcc)
set(CMAKE_AUTOUIC_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/autouic)

# 启用测试
enable_testing()

# 添加GTest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# 查找Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Multimedia MultimediaWidgets)
# 确保Qt6的moc工具可用
qt_standard_project_setup()

# 添加子目录
add_subdirectory(src/ffmpeg)

# 查找Boost库
find_package(Boost REQUIRED COMPONENTS system thread context)

# 收集源文件
set(SOURCES
    src/main.cpp
    src/ui/x264_config_window.cpp
)

# 添加编译优化选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -fopenmp")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -fopenmp")
endif()

# 查找必要的包
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 启用Qt自动生成
set_target_properties(${PROJECT_NAME} PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

# 添加包含目录
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${FFMPEG_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        cp_ffmpeg
        Boost::system
        Boost::thread
        Boost::context
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
        Qt6::Multimedia
        Qt6::MultimediaWidgets
        PkgConfig::FFMPEG
) 