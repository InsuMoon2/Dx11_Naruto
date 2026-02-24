pushd %~dp0
xcopy /y /d "Engine\Bin\Engine.dll" "Client\Bin\"
xcopy /y /d "Engine\Bin\Engine.dll" "Editor\Bin\"
xcopy /y /d "Engine\Bin\Engine.dll" "Game\Bin\"
xcopy /y /d "Engine\Bin\*.dll" "Game\Bin\"
xcopy /y /d "Engine\Bin\Engine.pdb" "Client\Bin\"        
xcopy /y /d "Engine\Bin\Engine.pdb" "Editor\Bin\"
xcopy /y /d "Engine\Bin\Engine.pdb" "Game\Bin\"   

xcopy /y /d "Client\Bin\Client.pdb" "Editor\Bin\"

xcopy /y /d "Engine\Bin\Engine.lib" "EngineSDK\Lib\"
xcopy /y /d /s "Engine\Public\*.*" "EngineSDK\Include\"
xcopy /y /d "Server\Protobuf\Bin\*.pb.h" "EngineSDK\Include\"
popd