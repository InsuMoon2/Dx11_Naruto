pushd %~dp0
xcopy /y /d "Engine\Bin\Engine.dll" "Client\Bin\"
xcopy /y /d "Engine\Bin\Engine.dll" "EditorApp\Bin\"
xcopy /y /d "Engine\Bin\Engine.lib" "EngineSDK\Lib\"
xcopy /y /d /s "Engine\Public\*.*" "EngineSDK\Include\"
xcopy /y /d "Server\Protobuf\Bin\*.pb.h" "EngineSDK\Include\"
popd