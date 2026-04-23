@echo off
setlocal

cd /d "%~dp0"

set "INPUT_DIR=%~dp0..\json\Levels"
set "PATTERN=BM_KonohaVillage*.proxy.level.json"
set "OUTPUT=%~dp0..\bin\CollisionSurfaceCache\KonohaVillage.surfacecache.bin"

echo [BakeKonohaSurfaceCache] input_dir=%INPUT_DIR%
echo [BakeKonohaSurfaceCache] pattern=%PATTERN%
echo [BakeKonohaSurfaceCache] output=%OUTPUT%

python "%~dp0BakeCollisionSurfaceCache.py" --input-dir "%INPUT_DIR%" --pattern "%PATTERN%" --output "%OUTPUT%"
if errorlevel 1 (
    echo.
    echo [BakeKonohaSurfaceCache] Failed.
    exit /b 1
)

echo.
echo [BakeKonohaSurfaceCache] Done.
endlocal
