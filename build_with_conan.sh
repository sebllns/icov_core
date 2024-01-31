#!/bin/bash

set -e
set -x

BASEDIR=$(dirname "$0")
pushd "$BASEDIR"

conan --version

mkdir -p build/conan/bin
cp -i conan/* build/
cd build

./configure | tee config.log

cmake --build . -j8 | tee icov_cmake.log
