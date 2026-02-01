@echo off
pushd %~dp0
set PROTOC=C:\vcpkg\installed\x64-windows\tools\protobuf\protoc.exe

if not exist "..\Bin" mkdir "..\Bin"
set OUTPUT_DIR=..\Bin
echo Generating Protocol files...
%PROTOC% -I=. --cpp_out=%OUTPUT_DIR% Enum.proto
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to generate Enum.pb files
    echo Check if protoc.exe exists at: %PROTOC%
    pause
    exit /b 1
)
%PROTOC% -I=. --cpp_out=%OUTPUT_DIR% Struct.proto
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to generate Struct.pb files
    pause
    exit /b 1
)
%PROTOC% -I=. --cpp_out=%OUTPUT_DIR% Protocol.proto
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to generate Protocol.pb files
    pause
    exit /b 1
)
echo.
echo ===================================
echo Protobuf generation complete!
echo ===================================
echo Generated files in: %OUTPUT_DIR%
dir ..\Bin\*.pb.*
pause
popd
