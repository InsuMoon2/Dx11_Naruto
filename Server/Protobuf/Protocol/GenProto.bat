@echo off
pushd %~dp0
set PROTOC=..\Bin\protoc.exe
set OUTPUT_DIR=..\Bin
%PROTOC% -I=. --cpp_out=%OUTPUT_DIR% Enum.proto
%PROTOC% -I=. --cpp_out=%OUTPUT_DIR% Struct.proto
%PROTOC% -I=. --cpp_out=%OUTPUT_DIR% Protocol.proto
echo Protobuf generation complete!
pause
popd