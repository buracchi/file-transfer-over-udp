cmake_minimum_required (VERSION 3.13)

include("${CMAKE_CURRENT_SOURCE_DIR}/.cmake/Vcpkg.cmake")
include(GoogleTest)

# Project Configuration

project (
	"File transfer over UDP"
	VERSION 1.0.0
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Guards

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(
    FATAL_ERROR
    "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
    )
endif()

# Dependencies

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/.dependencies" DEPENDENCY_LIST)
install_dependencies("${DEPENDENCY_LIST}")

# Test

option(BUILD_TESTING "Build the testing tree." ON)

if (BUILD_TESTING)
    enable_testing()
    find_package(GTest REQUIRED)
    # Notice that GTest::GTest and GTest::Main are deprecated since CMake 3.20 
    # and should be replaced with GTest::gtest and GTest::gtest_main but this
    # will make CMake 3.16.3 fails.
endif()

# Code Coverage Configuration

option(ENABLE_TEST_COVERAGE "Enable coverage reporting" OFF)

add_library(coverage_config INTERFACE)

if(ENABLE_TEST_COVERAGE)
  target_compile_options(coverage_config INTERFACE -O0 -g --coverage)
  target_link_options(coverage_config INTERFACE --coverage)
endif()

# Subproject

add_subdirectory("lib")
add_subdirectory("gbnd")
add_subdirectory("ft-server")
add_subdirectory("ft-client")
