#!/bin/sh
#
# This is a simple script to automate manual building of the project.
# This file is just an utility and it is not necessary to the project.
#
# To work with Visual Studio, the WSL installation or the remote Linux system
# must include the following tools: gdb, make, ninja-build, rsync, zip, cmake, g++
# and gcc or clang. In remote system is required to also install openssh-server.
#

BUILD_TYPE=release
PRESET=x64-linux-$BUILD_TYPE
BUILD_DIR=./out/build/x64-linux-$BUILD_TYPE

cmake -S . --preset $PRESET
cmake --build --preset $PRESET -j $(nproc)
echo "Build successfull. Executable are located inside $BUILD_DIR"
