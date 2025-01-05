project(cp_ui)

# 查找Qt包
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Test)

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

# 添加测试
set(TEST_SOURCES
    mainwindow_test.cpp
)

set(TEST_HEADERS
    mainwindow_test.hpp
)

add_executable(${PROJECT_NAME}_test ${TEST_SOURCES} ${TEST_HEADERS})

target_link_libraries(${PROJECT_NAME}_test
    PRIVATE
        ${PROJECT_NAME}
        Qt6::Core
        Qt6::Widgets
        Qt6::Test
)

# 启用测试
enable_testing()
add_test(NAME ${PROJECT_NAME}_test COMMAND ${PROJECT_NAME}_test) 