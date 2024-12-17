cmake_minimum_required(VERSION 3.10)
project(CrossPlatform VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(folly REQUIRED)
find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(gflags REQUIRED)

# Collect source files
set(SOURCES
    src/main.cpp
)

# Collect header file directories
set(INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/patterns
    ${PROJECT_SOURCE_DIR}/src/modern
    ${PROJECT_SOURCE_DIR}/src/threads
    ${PROJECT_SOURCE_DIR}/src/io
)

# Add executable file, including source files and header files
add_executable(${PROJECT_NAME} ${SOURCES})

# Add Link Library
target_link_libraries(${PROJECT_NAME} 
    folly
    ${Boost_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    gflags
    double-conversion
    glog
    event
    fmt 
)

# Add header file directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${INCLUDE_DIRS}
        /usr/local/include       
        ${Boost_INCLUDE_DIRS}     
        ${OPENSSL_INCLUDE_DIR}   
    ${GFLAGS_INCLUDE_DIR}     
) 