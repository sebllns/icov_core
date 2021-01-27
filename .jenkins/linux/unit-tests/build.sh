#!/bin/sh
cd ${ICOV_SRC_DIR}

mkdir build_tests
cd build_tests
cmake -DCMAKE_BUILD_TYPE=Debug ../icov_tests
make -j8
