@echo off
setlocal

rem MSVC Build Tools 2019 manual environment setup
set "MSVC_ROOT=C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Tools\MSVC\14.29.30133"
set "WINSDK_VER=10.0.26100.0"
set "WINSDK_ROOT=C:\Program Files (x86)\Windows Kits\10"

set "PATH=%MSVC_ROOT%\bin\Hostx64\x64;%WINSDK_ROOT%\bin\%WINSDK_VER%\x64;%PATH%"
set "INCLUDE=%MSVC_ROOT%\include;%WINSDK_ROOT%\Include\%WINSDK_VER%\ucrt;%WINSDK_ROOT%\Include\%WINSDK_VER%\shared;%WINSDK_ROOT%\Include\%WINSDK_VER%\um;%WINSDK_ROOT%\Include\%WINSDK_VER%\winrt;%WINSDK_ROOT%\Include\%WINSDK_VER%\cppwinrt"
set "LIB=%MSVC_ROOT%\lib\x64;%WINSDK_ROOT%\Lib\%WINSDK_VER%\ucrt\x64;%WINSDK_ROOT%\Lib\%WINSDK_VER%\um\x64"

pushd "%~dp0.."
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 goto :fail
cmake --build build
if errorlevel 1 goto :fail
ctest --test-dir build --output-on-failure
set RESULT=%ERRORLEVEL%
popd
exit /b %RESULT%

:fail
set RESULT=%ERRORLEVEL%
popd
exit /b %RESULT%
