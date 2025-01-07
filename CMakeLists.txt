cmake_minimum_required(VERSION 3.16)
project(CrossPlatform VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找Qt包
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets)

# 源文件
set(SOURCES
    src/main.cpp
    src/encode/x264_param_test.cpp
    src/encode/vp8_param_test.cpp
    src/format/aac_parser.cpp
    src/format/mp4_parser.cpp
    src/ui/main_window.cpp
    src/ui/x264_config_window.cpp
    src/ui/vp8_config_window.cpp
    src/ui/aac_config_window.cpp
    src/ui/aac_frame_view.cpp
    src/ui/mp4_config_window.cpp
    src/ui/mp4_box_view.cpp
)

# 头文件
set(HEADERS
    src/encode/x264_param_test.hpp
    src/encode/vp8_param_test.hpp
    src/format/aac_parser.hpp
    src/format/mp4_parser.hpp
    src/ui/main_window.hpp
    src/ui/x264_config_window.hpp
    src/ui/vp8_config_window.hpp
    src/ui/aac_config_window.hpp
    src/ui/aac_frame_view.hpp
    src/ui/mp4_config_window.hpp
    src/ui/mp4_box_view.hpp
)

# 查找依赖包
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
)

# 查找x264库
find_library(X264_LIBRARY x264)
if(NOT X264_LIBRARY)
    message(FATAL_ERROR "x264 library not found")
endif()

# 查找VPX库
find_library(VPX_LIBRARY vpx)
if(NOT VPX_LIBRARY)
    message(FATAL_ERROR "vpx library not found")
endif()

# 添加可执行文件
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
)

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::MultimediaWidgets
    PkgConfig::FFMPEG
    ${X264_LIBRARY}
    ${VPX_LIBRARY}
)

# 安装规则
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
) 