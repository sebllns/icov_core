#!/bin/sh
cd ./ICOV_SRC/build

valgrind -q --show-leak-kinds=all --track-origins=yes --leak-check=full ./icov_main  param1 param2 param3 