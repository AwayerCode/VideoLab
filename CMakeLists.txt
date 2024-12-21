cmake_minimum_required(VERSION 3.10)
project(CrossPlatform VERSION 1.0)

# 必须在 project() 之前设置
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")

# 自动下载并配置 vcpkg
include(FetchContent)
FetchContent_Declare(
    vcpkg
    GIT_REPOSITORY https://github.com/Microsoft/vcpkg.git
    GIT_TAG 4fcf123fe53c63db265b5841a5aeff06cec375a1
)
FetchContent_MakeAvailable(vcpkg)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt settings
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find packages
find_package(Boost COMPONENTS context REQUIRED)

# Collect source files
set(SOURCES
    src/main.cpp
)

set(INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/patterns
    ${PROJECT_SOURCE_DIR}/src/modern
    ${PROJECT_SOURCE_DIR}/src/threads
    ${PROJECT_SOURCE_DIR}/src/io
    ${PROJECT_SOURCE_DIR}/src/utils
    ${PROJECT_SOURCE_DIR}/src/template
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::context
)

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS}) 