#!/bin/bash

set -e
set -x

BASEDIR=$(dirname "$0")
pushd "$BASEDIR"

mkdir -p build/conan/bin
cd build
# we add -c tools.system.package_manager:mode=install if we want to install openGL
conan install .. --output-folder=conan --build=missing -c tools.system.package_manager:mode=install --settings compiler.libcxx="libstdc++11" 
cmake .. -DCMAKE_TOOLCHAIN_FILE=/home/sbelleno/ICOV/core/build/conan/build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release --log-level=TRACE -DICOV_USE_OMP=ON -DICOV_CORE_DEBUG_TRACE_ENABLED=ON -DICOV_CORE_STACKTRACE_ENABLED=ON -DICOV_CHECK_BITPLANE_ENCODING_ENABLED=OFF -DICOV_CONSOLE_COLOR_ENABLED=ON -DICOV_LOG_BOOST=OFF
cmake --build . -j8

