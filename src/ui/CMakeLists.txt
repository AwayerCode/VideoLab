project(cp_ui)

# 查找Qt包
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# 收集源文件
set(SOURCES
    mainwindow.cpp
)

set(HEADERS
    mainwindow.hpp
)

# 创建动态库
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})

# 设置包含目录
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接Qt库
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        cp_ffmpeg
) 