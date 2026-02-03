@echo off
pushd %~dp0
REM ============================================
REM vcpkg 경로 자동 감지 (여러 환경 대응)
REM ============================================
REM 1. 환경변수 VCPKG_ROOT가 설정되어 있는 경우
if defined VCPKG_ROOT (
    set PROTOC=%VCPKG_ROOT%\installed\x64-windows\tools\protobuf\protoc.exe
    goto :check_protoc
)
REM 2. D:\vcpkg 경로 확인 (현재 환경)
if exist "D:\vcpkg\installed\x64-windows\tools\protobuf\protoc.exe" (
    set PROTOC=D:\vcpkg\installed\x64-windows\tools\protobuf\protoc.exe
    goto :check_protoc
)
REM 3. C:\vcpkg 경로 확인 (이전 환경)
if exist "C:\vcpkg\installed\x64-windows\tools\protobuf\protoc.exe" (
    set PROTOC=C:\vcpkg\installed\x64-windows\tools\protobuf\protoc.exe
    goto :check_protoc
)
REM 4. PATH 환경변수에서 protoc 찾기
where protoc.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set PROTOC=protoc.exe
    goto :check_protoc
)
REM protoc를 찾을 수 없음
echo ERROR: protoc.exe not found!
echo.
echo Please install protobuf via vcpkg:
echo   vcpkg install protobuf:x64-windows
echo.
echo Or set VCPKG_ROOT environment variable to your vcpkg installation path.
pause
exit /b 1
:check_protoc
REM protoc.exe 존재 확인
if not exist "%PROTOC%" (
    echo ERROR: protoc.exe not found at: %PROTOC%
    echo Please check your vcpkg installation.
    pause
    exit /b 1
)
echo Found protoc at: %PROTOC%
echo.
REM 출력 디렉터리 생성
if not exist "..\Bin" mkdir "..\Bin"
set OUTPUT_DIR=..\Bin
REM ============================================
REM Protocol 파일 생성
REM ============================================
echo Generating Protocol files...
REM Enum.proto
echo [1/3] Generating Enum.pb...
%PROTOC% -I=. --cpp_out=%OUTPUT_DIR% Enum.proto
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to generate Enum.pb files
    pause
    exit /b 1
)
REM Struct.proto
echo [2/3] Generating Struct.pb...
%PROTOC% -I=. --cpp_out=%OUTPUT_DIR% Struct.proto
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to generate Struct.pb files
    pause
    exit /b 1
)
REM Protocol.proto
echo [3/3] Generating Protocol.pb...
%PROTOC% -I=. --cpp_out=%OUTPUT_DIR% Protocol.proto
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to generate Protocol.pb files
    pause
    exit /b 1
)
REM ============================================
REM 완료 메시지
REM ============================================
echo.
echo ===================================
echo Protobuf generation complete!
echo ===================================
echo Generated files in: %OUTPUT_DIR%
echo.
xcopy /y /d "%OUTPUT_DIR%\*.pb.h" "..\..\..\Engine\Public\"
popd
