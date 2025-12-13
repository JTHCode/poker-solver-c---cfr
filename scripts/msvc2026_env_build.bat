@echo off
setlocal

rem Make sure vswhere is on PATH so CMake can locate VS.
set "PATH=C:\Program Files (x86)\Microsoft Visual Studio\Installer;%PATH%"

pushd "%~dp0.."
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 goto :fail
cmake --build build --config Debug
if errorlevel 1 goto :fail
ctest --test-dir build --output-on-failure --config Debug
set RESULT=%ERRORLEVEL%
popd
exit /b %RESULT%

:fail
set RESULT=%ERRORLEVEL%
popd
exit /b %RESULT%
