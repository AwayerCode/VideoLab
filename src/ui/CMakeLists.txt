project(cp_ui)

# 查找Qt包
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Test Multimedia MultimediaWidgets)

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
        Qt6::Multimedia
        Qt6::MultimediaWidgets
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
        GTest::gtest
        GTest::gtest_main
)

# 启用测试
include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_test) 