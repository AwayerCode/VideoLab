cmake_minimum_required(VERSION 3.16)
project(CrossPlatform VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置自动处理 Qt MOC、UIC、RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找OpenMP
find_package(OpenMP REQUIRED)

# 设置编译选项
add_compile_options(-O3 -march=native -fopenmp)

# 查找必要的包
find_package(Qt6 COMPONENTS Core Widgets Multimedia MultimediaWidgets REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libavfilter
    libswscale
    libswresample
)

# 设置头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# 设置所有源文件
set(PROJECT_SOURCES
    src/main.cpp
    src/encode/x264_param_test.cpp
    src/encode/x265_param_test.cpp
    src/ui/x264_config_window.cpp
    src/ui/main_window.cpp
    src/format/mp4_parser.cpp
    src/format/aac_parser.cpp
    src/ui/mp4_config_window.cpp
    src/ui/mp4_box_view.cpp
    src/ui/aac_config_window.cpp
    src/ui/aac_frame_view.cpp
)

# 设置所有头文件
set(PROJECT_HEADERS
    src/ui/main_window.hpp
    src/ui/x264_config_window.hpp
    src/encode/x264_param_test.hpp
    src/encode/x265_param_test.hpp
    src/format/mp4_parser.hpp
    src/format/aac_parser.hpp
    src/ui/mp4_config_window.hpp
    src/ui/mp4_box_view.hpp
    src/ui/aac_config_window.hpp
    src/ui/aac_frame_view.hpp
)

# 创建可执行文件
add_executable(${PROJECT_NAME} 
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
)

# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FFMPEG_INCLUDE_DIRS}
)

# 设置链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    boost_system
    boost_thread
    boost_context
    boost_atomic
    OpenMP::OpenMP_CXX
    x264
    ${FFMPEG_LIBRARIES}
) 