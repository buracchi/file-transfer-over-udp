cmake_minimum_required (VERSION 3.8)

# Submodules Configuration

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Updating git submodules...")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

# Vcpkg Configuration

set(VCPKG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg")

if(NOT EXISTS "${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake")
    message(FATAL_ERROR "The vcpkg submodules was not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()

if(NOT EXISTS "${VCPKG_DIR}/vcpkg" AND NOT EXISTS "${VCPKG_DIR}/vcpkg.exe")
    message(STATUS "Installing vcpkg...")
    if (${UNIX})
        set(FORMAT "sh")
    elseif(${WIN32})
        set(FORMAT "bat")
    else()
        message(FATAL_ERROR "Vcpkg cannot be insalled in this operating system.")
    endif()
    exec_program(
        ${VCPKG_DIR}/bootstrap-vcpkg.${FORMAT} ${VCPKG_DIR}
    )
endif()

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Using Vcpkg toolchain")
    set(CMAKE_TOOLCHAIN_FILE
        "${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file"
    )
endif()

include("${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake")

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

# Dependencies Configuration

message(STATUS "Installing dependencies with vcpkg...")

set(TRIPLET "x64-linux")
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/.dependencies DEPENDENCY_LIST)
foreach(DEPENDENCY IN LISTS DEPENDENCY_LIST)
    find_package(${DEPENDENCY} QUIET)
    if (NOT ${${DEPENDENCY}_FOUND})
            exec_program(${VCPKG_DIR}/vcpkg ARGS "install ${DEPENDENCY}:${TRIPLET}")
    endif()
endforeach()

# Subproject

add_subdirectory ("lib")
add_subdirectory ("client")
add_subdirectory ("server")
