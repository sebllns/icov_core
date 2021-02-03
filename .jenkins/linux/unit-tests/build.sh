#!/bin/sh
mkdir build-tests
cd build-tests
cmake -DCMAKE_BUILD_TYPE=Debug ../tests
make -j8
