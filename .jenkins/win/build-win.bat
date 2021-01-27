cd %ICOV_SRC_DIR%
mkdir build
cd build
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
cmake -DCMAKE_BUILD_TYPE=Debug ../icov_main
devenv icov_main.sln /build Debug
