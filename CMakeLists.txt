cmake_minimum_required(VERSION 3.10)
project(EventProcessor LANGUAGES CXX)

# Set C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default to Debug build type if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Include directories: current directory and the "src" folder
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Source files
set(SOURCES
    main.cpp
    src/benchmark.cpp
    src/ring_buffer.cpp
)

# Create the executable target
add_executable(${PROJECT_NAME} ${SOURCES})

# Compiler flags for Debug and Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Debug flags: no optimization, full warnings, address sanitizer, debug info
    target_compile_options(${PROJECT_NAME} PRIVATE
        -O0
        -Wall
        -Wextra
        -Wpedantic
        -fdiagnostics-color=always
        -fconcepts
        -g
        -ggdb3
        -fsanitize=address
        -DDEBUG  # Added DEBUG flag for Debug builds
    )
    target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Release flags: high optimization, native march, warnings
    target_compile_options(${PROJECT_NAME} PRIVATE
        -O3
        -march=native
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# Linker flags: link with pthread
target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
