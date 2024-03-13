#!/bin/bash

set -e
set -x

BASEDIR=$(dirname "$0")
pushd "$BASEDIR"


conan --version
mkdir -p build/conan/bin
cp -vf conan/* build/
cd build

read -p "[1/2] Press enter to configure ICOV project..."
./configure | tee config.log


read -p "[2/2] Press enter to build ICOV binaries..."
cmake --build . -j8 | tee build.log
