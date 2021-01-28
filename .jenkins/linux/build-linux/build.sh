#!/bin/sh
mkdir ${BUILD_DIR}
cd ${BUILD_DIR}
cmake -DCMAKE_BUILD_TYPE=Debug ..
make clean
make -j8