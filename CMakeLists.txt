cmake_minimum_required (VERSION 3.21)

option(BUILD_TESTS "Build test programs" ON)
option(ENABLE_TEST_COVERAGE "Enable coverage reporting" OFF)
option(BUILD_DOC "Build documentation" OFF)

if (BUILD_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
    include(GoogleTest)
endif ()

project("File transfer over UDP" VERSION 1.0)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Guards

if (PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(
        FATAL_ERROR
        "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
    )
endif ()

# Test

if (BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
endif ()

# Code Coverage Configuration

add_library(coverage_config INTERFACE)

if (ENABLE_TEST_COVERAGE)
    target_compile_options(coverage_config INTERFACE -O0 -g --coverage)
    target_link_options(coverage_config INTERFACE --coverage)
endif ()

# Subproject

add_subdirectory("lib")
add_subdirectory("ft-server")
add_subdirectory("client")
#add_subdirectory("gbn")
