cmake_minimum_required(VERSION 3.10)
project(rle VERSION 1.0.0 LANGUAGES CXX)

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

# RLE library
add_library(rle_lib rle.cpp rle.hpp)
target_include_directories(rle_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Consolidated test executable
add_executable(test_rle test_rle.cpp)
target_link_libraries(test_rle PRIVATE rle_lib)

# Comprehensive verification test
add_executable(test_comprehensive test_comprehensive.cpp)
target_link_libraries(test_comprehensive PRIVATE rle_lib)

# Ground truth comparison tool (for analysis only, not a test)
add_executable(test_ground_truth test_ground_truth.cpp)
target_link_libraries(test_ground_truth PRIVATE rle_lib)

# Optional: Fuzz test executable (disabled by default, run manually)
option(ENABLE_FUZZ_TESTS "Build fuzz test executable" OFF)
if(ENABLE_FUZZ_TESTS)
    add_executable(test_fuzz test_fuzz.cpp)
    target_link_libraries(test_fuzz PRIVATE rle_lib)
endif()

# Enable testing
enable_testing()
add_test(NAME rle_comprehensive COMMAND test_rle)
add_test(NAME rle_verification COMMAND test_comprehensive)

# Optional: Add code coverage support (requires GCC or Clang)
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(rle_lib PUBLIC --coverage)
        target_link_options(rle_lib PUBLIC --coverage)
        target_compile_options(test_rle PRIVATE --coverage)
        target_link_options(test_rle PRIVATE --coverage)
    endif()
endif()
