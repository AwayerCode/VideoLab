project(cp_ffmpeg)

# 查找 FFmpeg 库
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
    libavfilter
)

# 收集源文件
set(SOURCES
    ffmpeg.cpp
    encoder_test.cpp
    x264_param_test.cpp
    videofilter.cpp
)

set(HEADERS
    ffmpeg.hpp
    encoder_test.hpp
    x264_param_test.hpp
    videofilter.hpp
)

# 创建动态库
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})

# 设置包含目录
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${FFMPEG_INCLUDE_DIRS}
)

# 链接 FFmpeg 库
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        PkgConfig::FFMPEG
) 