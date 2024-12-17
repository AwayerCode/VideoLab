cmake_minimum_required(VERSION 3.10)
project(CrossPlatform VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add Boost Library
find_package(Boost REQUIRED COMPONENTS 
    system 
    filesystem
)

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
)

# Add executable file, including source files and header files
add_executable(${PROJECT_NAME} ${SOURCES})

# Add Link Library
target_link_libraries(${PROJECT_NAME} 
    ${Boost_LIBRARIES}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
)

# Add header file directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}
) 