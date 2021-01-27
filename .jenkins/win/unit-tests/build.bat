cd %ICOV_SRC_DIR%

mkdir build_tests
cd build_tests
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
cmake -DCMAKE_BUILD_TYPE=Debug -DBOOST_ROOT=Z:\local\boost_1_75_0 -DBOOST_LIBRARYDIR=Z:\local\boost_1_75_0\lib64-msvc-14.1 ../icov_tests
devenv icov_tests.sln /build Debug

