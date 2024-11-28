# 添加可执行文件
add_executable(my_program main.cpp)

# 添加头文件目录
target_include_directories(my_program
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
) 