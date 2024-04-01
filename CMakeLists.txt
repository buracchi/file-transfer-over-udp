cmake_minimum_required (VERSION 3.25)

option(BUILD_TESTS "Build test programs" ON)
option(ENABLE_TEST_COVERAGE "Enable coverage reporting" OFF)
option(BUILD_DOC "Build documentation" OFF)

if (BUILD_TESTS)
	list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif ()

project("File transfer over UDP"
		LANGUAGES C
		VERSION 1.0)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Test

if (BUILD_TESTS)
	enable_testing()
endif ()

add_library(coverage_config INTERFACE)

if (ENABLE_TEST_COVERAGE)
	target_compile_options(coverage_config INTERFACE -O0 -g --coverage)
	target_link_options(coverage_config INTERFACE --coverage)
endif ()

# Subproject

#add_subdirectory("ftcp")
add_subdirectory("network_utils")
add_subdirectory("server")
add_subdirectory("client")
