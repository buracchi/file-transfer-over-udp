#!/bin/sh
#
# This is a simple script to automate manual building of the project.
# This file is just an utility and it is not necessary to the project.
#

BUILD_TYPE=Release

cmake -S . -B build -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_C_FLAGS="-fsanitize=address"
cmake --build build -j $(nproc) --config $BUILD_TYPE
