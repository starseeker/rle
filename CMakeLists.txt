cmake_minimum_required(VERSION 3.10)
project(rle VERSION 1.0.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Build utahrle submodule
set(BUILD_SHARED_LIBS OFF)
set(BUILD_STATIC_LIBS ON)
add_subdirectory(utahrle EXCLUDE_FROM_ALL)

# RLE library
add_library(rle_lib rle.cpp rle.hpp)
target_include_directories(rle_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Test executable
add_executable(test_rle test_rle.cpp)
target_link_libraries(test_rle PRIVATE rle_lib utahrle-static)
target_include_directories(test_rle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/utahrle/include)

# Enable testing
enable_testing()
add_test(NAME test_rle COMMAND test_rle)
