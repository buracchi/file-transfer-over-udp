#!/bin/sh
#
# This is a simple script to automate manual building of the project.
# This file is just an utility and it is not necessary to the project.
#
# To work with Visual Studio, the WSL installation or the remote Linux system
# must include the following tools: gdb, make, ninja-build, rsync, zip, cmake, g++
# and gcc or clang. In remote system is required to also install openssh-server.
#

BUILD_TYPE=Release

cmake -S . -B build -DCMAKE_BUILD_TYPE=$BUILD_TYPE
cmake --build build -j $(nproc) --config $BUILD_TYPE
