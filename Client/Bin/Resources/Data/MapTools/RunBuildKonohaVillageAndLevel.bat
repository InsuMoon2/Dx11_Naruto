@echo off
setlocal EnableExtensions
cd /d "%~dp0"

REM Restore default input paths even when outer environment variables are blank.
if "%KNV_MESH_SRC%"=="" set "KNV_MESH_SRC=D:\NarutoExports\Game\Environments\MapAssets\konohaVillage\Meshes"
if "%KNV_MI_ROOT%"=="" set "KNV_MI_ROOT=D:\NarutoExports\Game\Environments\MapAssets\konohaVillage\Materials"
if "%KNV_TEXTURE_ROOT%"=="" set "KNV_TEXTURE_ROOT=D:\NarutoExports\Game\Environments\MapAssets\konohaVillage\Textures"
if "%KNV_LEVEL_MAP_ROOT%"=="" set "KNV_LEVEL_MAP_ROOT=C:\Users\moon\Desktop\Output\Exports\NARUTO\Content\Maps\BattleMaps\KonohaVillage"

set "KNV_MESH_DST=..\..\StaticMesh\KonohaVillage\Meshes"
set "KNV_TEX_DST=..\..\Textures\FModel\KonohaVillage"
set "KNV_MATINST_DST=..\..\Materials\FModel\KonohaVillage"
set "KNV_GUID_MAP=..\json\KonohaVillage_mesh_guid_map.json"
set "KNV_LEVEL_OUT=..\json\Levels"

if not defined ASSIMP_TOOL set "ASSIMP_TOOL=%~dp0..\..\..\..\..\AssimpTool\Bin\AssimpTool.exe"

REM Fail fast when a required input path is missing.
if not exist "%KNV_MESH_SRC%" (
  echo [ERROR] KNV_MESH_SRC not found: %KNV_MESH_SRC%
  goto :error_exit
)

if not exist "%KNV_MI_ROOT%" (
  echo [ERROR] KNV_MI_ROOT not found: %KNV_MI_ROOT%
  goto :error_exit
)

if not exist "%KNV_TEXTURE_ROOT%" (
  echo [ERROR] KNV_TEXTURE_ROOT not found: %KNV_TEXTURE_ROOT%
  goto :error_exit
)

if not exist "%KNV_LEVEL_MAP_ROOT%" (
  echo [ERROR] KNV_LEVEL_MAP_ROOT not found: %KNV_LEVEL_MAP_ROOT%
  goto :error_exit
)

echo [RunBuildKonohaVillageAndLevel] Working directory: %CD%
echo   KNV_MESH_SRC=%KNV_MESH_SRC%
echo   KNV_MI_ROOT=%KNV_MI_ROOT%
echo   KNV_TEXTURE_ROOT=%KNV_TEXTURE_ROOT%
echo   KNV_LEVEL_MAP_ROOT=%KNV_LEVEL_MAP_ROOT%
echo   KNV_MESH_DST=%KNV_MESH_DST%
echo   KNV_TEX_DST=%KNV_TEX_DST%
echo   KNV_MATINST_DST=%KNV_MATINST_DST%
echo   KNV_GUID_MAP=%KNV_GUID_MAP%
echo   KNV_LEVEL_OUT=%KNV_LEVEL_OUT%
echo   ASSIMP_TOOL=%ASSIMP_TOOL%

python "%~dp0BuildKonohaVillagePipeline.py" ^
  --assimp-tool "%ASSIMP_TOOL%" ^
  --mesh-src "%KNV_MESH_SRC%" ^
  --mesh-dst "%KNV_MESH_DST%" ^
  --mi-root "%KNV_MI_ROOT%" ^
  --texture-root "%KNV_TEXTURE_ROOT%" ^
  --copy-textures-to "%KNV_TEX_DST%" ^
  --matinst-root "%KNV_MATINST_DST%" ^
  --level-map-root "%KNV_LEVEL_MAP_ROOT%" ^
  --guid-map-out "%KNV_GUID_MAP%" ^
  --level-out-dir "%KNV_LEVEL_OUT%" ^
  %*

if errorlevel 1 goto :error_exit

echo.
echo [RunBuildKonohaVillageAndLevel] Finished.
pause
endlocal
goto :eof

:error_exit
echo.
echo [RunBuildKonohaVillageAndLevel] Aborted.
pause
endlocal
exit /b 1
