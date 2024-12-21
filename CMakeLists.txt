cmake_minimum_required(VERSION 3.10)
project(CrossPlatform VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt settings
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find packages
find_package(fmt CONFIG REQUIRED)
find_package(folly CONFIG REQUIRED)
find_package(gflags CONFIG REQUIRED)
find_package(glog CONFIG REQUIRED)
find_package(Qt6 COMPONENTS Core Widgets REQUIRED)
find_package(Boost COMPONENTS context REQUIRED)

# Collect source files
set(SOURCES
    src/main.cpp
    src/threads/signalslot.cpp
    src/threads/signalslot.hpp
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
    fmt::fmt
    folly
    gflags
    glog::glog
    Qt6::Core
    Qt6::Widgets
    Boost::context
)

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS}) 