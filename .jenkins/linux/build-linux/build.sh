#!/bin/sh
cd ${ICOV_SRC_DIR}

mkdir ${BUILD_DIR}
cd ${BUILD_DIR}
cmake -DCMAKE_BUILD_TYPE=Debug ../${ICOV_MAIN_PROJECT_DIR}
make -j8