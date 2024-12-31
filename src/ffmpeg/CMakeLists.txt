project(cp_ffmpeg)

# 查找 FFmpeg 库
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
)

# 收集源文件
set(SOURCES
    ffmpeg.cpp
)

set(HEADERS
    ffmpeg.hpp
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