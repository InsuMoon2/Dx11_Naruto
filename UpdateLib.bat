pushd %~dp0
xcopy /y /d "Bin\Engine.dll" "Client\Bin\"
xcopy /y /d "Bin\Engine.lib" "EngineSDK\Lib\"
xcopy /y /d /s "Engine\Public\*.*" "EngineSDK\Include\"
xcopy /y /d "Server\Protobuf\Bin\*.pb.h" "EngineSDK\Include\"
popd