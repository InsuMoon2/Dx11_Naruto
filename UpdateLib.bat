pushd %~dp0
xcopy /y "Engine\Bin\Engine.dll" "Client\Bin\"
xcopy /y "Engine\Bin\Engine.dll" "Editor\Bin\"
xcopy /y "Engine\Bin\Engine.dll" "Game\Bin\"
xcopy /y "Engine\Bin\fmod*.dll" "Editor\Bin\"
xcopy /y "Engine\Bin\fmod*.dll" "Game\Bin\"
xcopy /y "Engine\Bin\*.dll" "Game\Bin\"
xcopy /y "Engine\Bin\Engine.pdb" "Client\Bin\"        
xcopy /y "Engine\Bin\Engine.pdb" "Editor\Bin\"
xcopy /y "Engine\Bin\Engine.pdb" "Game\Bin\"   

xcopy /y "Client\Bin\Client.pdb" "Editor\Bin\"

xcopy /y "Engine\Bin\Engine.lib" "EngineSDK\Lib\"
xcopy /y /s "Engine\Public\*.*" "EngineSDK\Include\"
xcopy /y "Server\Protobuf\Bin\*.pb.h" "EngineSDK\Include\"
popd
