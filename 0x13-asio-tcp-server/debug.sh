#!/bin/bash
set -e

# delete the build directory if it exists
if [ -d "build" ]; then
    rm -rf build
fi

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DDEBUG_TRACE=ON
cmake --build build --config Debug --target all -j 10
