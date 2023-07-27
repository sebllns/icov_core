#!/bin/bash

set -e


BASEDIR=$(dirname "$0")
pushd "$BASEDIR"

if ! command -v conda &> /dev/null
then
    echo "FATAL: conda could not be found"
    exit 1
fi

set -x

rm -rf build
mkdir build
cd build

my_env=icov


conda deactivate
conda remove --name $my_env --all
conda create --name $my_env conan
conda activate $my_env

