project(cp_ffmpeg)

# 查找 FFmpeg 库
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libavfilter
    libswscale
)

# 收集源文件
set(SOURCES
    ffmpeg.cpp
    analyzer.cpp
    videofilter.cpp
    x264_param_test.cpp
    x265_param_test.cpp
    encoder_test.cpp
)

# 创建库
add_library(cp_ffmpeg ${SOURCES})

# 链接 FFmpeg 库
target_link_libraries(cp_ffmpeg
    PRIVATE
        PkgConfig::FFMPEG
)

# 设置包含目录
target_include_directories(cp_ffmpeg
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
) 