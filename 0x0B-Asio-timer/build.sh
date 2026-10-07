#!/bin/bash
set -e

cd "$(dirname "$0")"

if [ ! -d build ]; then
    mkdir build
fi

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j "$(nproc)"
