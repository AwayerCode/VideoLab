cmake_minimum_required(VERSION 3.10)
project(CrossPlatform VERSION 1.0)

# 使用 FetchContent 管理依赖
include(FetchContent)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt settings
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 首先尝试查找系统安装的 gflags
find_package(gflags QUIET)

# 只有在系统中没找到 gflags 时才使用 FetchContent
if(NOT gflags_FOUND)
    FetchContent_Declare(
        gflags
        GIT_REPOSITORY https://github.com/gflags/gflags.git
        GIT_TAG v2.2.2
    )
    FetchContent_MakeAvailable(gflags)
endif()

# 其他依赖
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 9.1.0
)

FetchContent_Declare(
    folly
    GIT_REPOSITORY https://github.com/facebook/folly.git
    GIT_TAG v2023.10.09.00
)

FetchContent_Declare(
    boost
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG boost-1.83.0
)

# 使依赖可用
FetchContent_MakeAvailable(fmt)

# Folly 需要特殊处理因为它有额外的依赖
FetchContent_GetProperties(folly)
if(NOT folly_POPULATED)
    FetchContent_Populate(folly)
    set(BUILD_TESTS OFF CACHE BOOL "")
    add_subdirectory(${folly_SOURCE_DIR} ${folly_BINARY_DIR})
endif()

# Boost 需要特殊处理
FetchContent_GetProperties(boost)
if(NOT boost_POPULATED)
    FetchContent_Populate(boost)
    set(BOOST_ROOT ${boost_SOURCE_DIR})
    set(Boost_NO_SYSTEM_PATHS ON)
    find_package(Boost REQUIRED)
endif()

# Find Qt
find_package(Qt6 REQUIRED COMPONENTS Core)

# Collect source files
set(SOURCES
    src/main.cpp
    src/threads/signalslot.cpp
    src/threads/signalslot.hpp
)

# Collect header file directories
set(INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/patterns
    ${PROJECT_SOURCE_DIR}/src/modern
    ${PROJECT_SOURCE_DIR}/src/threads
    ${PROJECT_SOURCE_DIR}/src/io
    ${PROJECT_SOURCE_DIR}/src/utils
    ${PROJECT_SOURCE_DIR}/src/template
)

# Add executable file, including source files and header files
add_executable(${PROJECT_NAME} ${SOURCES})

# Add Link Library
target_link_libraries(${PROJECT_NAME} 
    folly
    fmt::fmt
    gflags
    ${Boost_LIBRARIES}
    Qt6::Core
)

# Add header file directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}
) 