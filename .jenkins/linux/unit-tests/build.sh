#!/bin/sh

rm -rf build-tests

mkdir build-tests
cd build-tests
cmake -DCMAKE_BUILD_TYPE=Debug ../tests
make -j8
