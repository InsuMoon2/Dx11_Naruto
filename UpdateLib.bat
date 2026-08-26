@echo off
setlocal

rem Build configuration to sync (Debug / Debug_Unity / Release). Defaults to Debug.
set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Debug

pushd %~dp0

if not exist "Client\Bin\%CONFIG%" mkdir "Client\Bin\%CONFIG%"
if not exist "Editor\Bin\%CONFIG%" mkdir "Editor\Bin\%CONFIG%"
if not exist "Game\Bin\%CONFIG%" mkdir "Game\Bin\%CONFIG%"

xcopy /y "Engine\Bin\%CONFIG%\Engine.dll" "Client\Bin\%CONFIG%\"
xcopy /y "Engine\Bin\%CONFIG%\Engine.dll" "Editor\Bin\%CONFIG%\"
xcopy /y "Engine\Bin\%CONFIG%\Engine.dll" "Game\Bin\%CONFIG%\"
xcopy /y "Engine\Bin\fmod*.dll" "Editor\Bin\%CONFIG%\"
xcopy /y "Engine\Bin\fmod*.dll" "Game\Bin\%CONFIG%\"
xcopy /y "Engine\Bin\%CONFIG%\*.dll" "Game\Bin\%CONFIG%\"
xcopy /y "Engine\Bin\%CONFIG%\Engine.pdb" "Client\Bin\%CONFIG%\"
xcopy /y "Engine\Bin\%CONFIG%\Engine.pdb" "Editor\Bin\%CONFIG%\"
xcopy /y "Engine\Bin\%CONFIG%\Engine.pdb" "Game\Bin\%CONFIG%\"

xcopy /y "Client\Bin\%CONFIG%\Client.pdb" "Editor\Bin\%CONFIG%\"

xcopy /y "Engine\Bin\%CONFIG%\Engine.lib" "EngineSDK\Lib\"
xcopy /y /s "Engine\Public\*.*" "EngineSDK\Include\"
xcopy /y "Server\Protobuf\Bin\*.pb.h" "EngineSDK\Include\"

xcopy /y "Engine\Bin\Shaders\*.hlsli" "Client\Bin\Shaders\"
xcopy /y "Engine\Bin\Shaders\*.hlsli" "Editor\Bin\Shaders\"

popd
