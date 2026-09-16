@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64
cd /d "%~dp0build"
cmake --build . 2>&1
echo === BUILD DONE ===
