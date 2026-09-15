@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64
cd /d "%~dp0"
if exist build rmdir /s /q build
mkdir build
cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe -DCMAKE_PREFIX_PATH="C:\Qt\6.4.3\msvc2019_64"
cmake --build .
echo.
echo === BUILD COMPLETE ===
pause
