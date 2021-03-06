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

set(IS_UNIX FALSE)
set(IS_WINDOWS FALSE)
if (${UNIX})
    set(IS_UNIX TRUE)
elseif(${WIN32})
    set(IS_WINDOWS TRUE)
endif()

set(VCPKG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg")

if(NOT EXISTS "${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake")
    message(FATAL_ERROR "The vcpkg submodules was not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()

if((${IS_UNIX} AND NOT EXISTS "${VCPKG_DIR}/vcpkg") OR (${IS_WINDOWS} AND NOT EXISTS "${VCPKG_DIR}/vcpkg.exe"))
    message(STATUS "Installing vcpkg...")
    if (${UNIX})
        set(FORMAT "sh")
    elseif(${WIN32})
        set(FORMAT "bat")
    else()
        message(FATAL_ERROR "Vcpkg cannot be insalled in this operating system.")
    endif()
    exec_program(${VCPKG_DIR}/bootstrap-vcpkg.${FORMAT} ${VCPKG_DIR})
endif()

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Using Vcpkg toolchain")
    set(CMAKE_TOOLCHAIN_FILE
        "${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file"
    )
endif()

# Vcpkg functions

function(install_dependencies)
    message(STATUS "Installing dependencies with vcpkg...")
    foreach(DEPENDENCY IN LISTS ARGV)
        find_package(${DEPENDENCY} QUIET)
        if (NOT ${${DEPENDENCY}_FOUND})
            message(STATUS "Installing ${DEPENDENCY}...")
            exec_program(${VCPKG_DIR}/vcpkg ARGS "install ${DEPENDENCY}")
        endif()
    endforeach()
endfunction()
