@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

mkdir build
cd build
cmake -G "Visual Studio 17 2022" ..
cmake --build . --config Release
pause