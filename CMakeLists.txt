cmake_minimum_required (VERSION 3.8)

include("${CMAKE_CURRENT_SOURCE_DIR}/.cmake/Vcpkg.cmake")

# Project Configuration

project (
	"File transfer over UDP"
	VERSION 1.0.0
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Guards

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(
    FATAL_ERROR
    "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
    )
endif()

# Dependencies

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/.dependencies" DEPENDENCY_LIST)
install_dependencies(DEPENDENCY_LIST)

# Test

option (BUILD_TESTING "Build the testing tree." ON)

if (BUILD_TESTING)
    enable_testing()
    message("Test enabled")
endif()

# Code Coverage Configuration

add_library(coverage_config INTERFACE)

option(CODE_COVERAGE "Enable coverage reporting" OFF)

if(CODE_COVERAGE AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    # Add required flags (GCC & LLVM/Clang)
    target_compile_options(
        coverage_config INTERFACE
        -O0        # no optimization
        -g         # generate debug info
        --coverage # sets all required flags
    )
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
        target_link_options(coverage_config INTERFACE --coverage)
    else()
        target_link_libraries(coverage_config INTERFACE --coverage)
    endif()
endif()

# Subproject

add_subdirectory("lib")
add_subdirectory("client")
add_subdirectory("server")
