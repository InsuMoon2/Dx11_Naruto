@echo off
setlocal EnableExtensions
cd /d "%~dp0"

REM Restore default input paths even when outer environment variables are blank.
if "%KNV02_MESH_SRC%"=="" set "KNV02_MESH_SRC=D:\NarutoExports\Game\Environments\MapAssets\KonohaVillage02\Meshes"
if "%KNV02_MI_ROOT%"=="" set "KNV02_MI_ROOT=D:\NarutoExports\Game\Environments\MapAssets\KonohaVillage02\Materials"
if "%KNV02_TEXTURE_ROOT%"=="" set "KNV02_TEXTURE_ROOT=D:\NarutoExports\Game\Environments\MapAssets\KonohaVillage02\Textures"
REM KonohaVillage02 materials also reference legacy konohaVillage textures.
if "%KNV02_EXTRA_TEXTURE_ROOT_1%"=="" set "KNV02_EXTRA_TEXTURE_ROOT_1=D:\NarutoExports\Game\Environments\MapAssets\konohaVillage\Textures"
REM Some floor and lobby materials also reference LobbyMapAssets textures.
if "%KNV02_EXTRA_TEXTURE_ROOT_2%"=="" set "KNV02_EXTRA_TEXTURE_ROOT_2=D:\NarutoExports\Game\Environments\LobbyMapAssets\LM_KonohaVillage_BORUTO\Textures"
if "%KNV02_LEVEL_MAP_ROOT%"=="" set "KNV02_LEVEL_MAP_ROOT=D:\NarutoExports\MapJSON\KonohaVillage02"

set "KNV02_MESH_DST=..\..\StaticMesh\KonohaVillage02\Meshes"
set "KNV02_TEX_DST=..\..\Textures\FModel\KonohaVillage02"
set "KNV02_MATINST_DST=..\..\Materials\FModel\KonohaVillage02"
set "KNV02_GUID_MAP=..\json\KonohaVillage02_mesh_guid_map.json"
set "KNV02_LEVEL_OUT=..\json\Levels"
REM Snow blend textures are not needed for this KonohaVillage02 conversion preset.
set "KNV02_STRIP_SNOW_ARG=--strip-snow"

if not defined ASSIMP_TOOL set "ASSIMP_TOOL=%~dp0..\..\..\..\..\AssimpTool\Bin\AssimpTool.exe"

REM Fail fast when a required input path is missing.
if not exist "%KNV02_MESH_SRC%" (
  echo [ERROR] KNV02_MESH_SRC not found: %KNV02_MESH_SRC%
  goto :error_exit
)

if not exist "%KNV02_MI_ROOT%" (
  echo [ERROR] KNV02_MI_ROOT not found: %KNV02_MI_ROOT%
  goto :error_exit
)

if not exist "%KNV02_TEXTURE_ROOT%" (
  echo [ERROR] KNV02_TEXTURE_ROOT not found: %KNV02_TEXTURE_ROOT%
  goto :error_exit
)

if not exist "%KNV02_EXTRA_TEXTURE_ROOT_1%" (
  echo [ERROR] KNV02_EXTRA_TEXTURE_ROOT_1 not found: %KNV02_EXTRA_TEXTURE_ROOT_1%
  goto :error_exit
)

if not exist "%KNV02_EXTRA_TEXTURE_ROOT_2%" (
  echo [ERROR] KNV02_EXTRA_TEXTURE_ROOT_2 not found: %KNV02_EXTRA_TEXTURE_ROOT_2%
  goto :error_exit
)

if not exist "%KNV02_LEVEL_MAP_ROOT%" (
  echo [ERROR] KNV02_LEVEL_MAP_ROOT not found: %KNV02_LEVEL_MAP_ROOT%
  goto :error_exit
)

echo [RunBuildKonohaVillage02AndLevel_fixed] Working directory: %CD%
echo   KNV02_MESH_SRC=%KNV02_MESH_SRC%
echo   KNV02_MI_ROOT=%KNV02_MI_ROOT%
echo   KNV02_TEXTURE_ROOT=%KNV02_TEXTURE_ROOT%
echo   KNV02_EXTRA_TEXTURE_ROOT_1=%KNV02_EXTRA_TEXTURE_ROOT_1%
echo   KNV02_EXTRA_TEXTURE_ROOT_2=%KNV02_EXTRA_TEXTURE_ROOT_2%
echo   KNV02_LEVEL_MAP_ROOT=%KNV02_LEVEL_MAP_ROOT%
echo   KNV02_MESH_DST=%KNV02_MESH_DST%
echo   KNV02_TEX_DST=%KNV02_TEX_DST%
echo   KNV02_MATINST_DST=%KNV02_MATINST_DST%
echo   KNV02_GUID_MAP=%KNV02_GUID_MAP%
echo   KNV02_LEVEL_OUT=%KNV02_LEVEL_OUT%
echo   KNV02_STRIP_SNOW_ARG=%KNV02_STRIP_SNOW_ARG%
echo   ASSIMP_TOOL=%ASSIMP_TOOL%

python "%~dp0BuildKonohaVillage02Pipeline.py" ^
  --assimp-tool "%ASSIMP_TOOL%" ^
  --mesh-src "%KNV02_MESH_SRC%" ^
  --mesh-dst "%KNV02_MESH_DST%" ^
  --mi-root "%KNV02_MI_ROOT%" ^
  --texture-root "%KNV02_TEXTURE_ROOT%" ^
  --extra-texture-root "%KNV02_EXTRA_TEXTURE_ROOT_1%" ^
  --extra-texture-root "%KNV02_EXTRA_TEXTURE_ROOT_2%" ^
  --copy-textures-to "%KNV02_TEX_DST%" ^
  --matinst-root "%KNV02_MATINST_DST%" ^
  --level-map-root "%KNV02_LEVEL_MAP_ROOT%" ^
  --guid-map-out "%KNV02_GUID_MAP%" ^
  --level-out-dir "%KNV02_LEVEL_OUT%" ^
  %KNV02_STRIP_SNOW_ARG% ^
  %*

if errorlevel 1 goto :error_exit

echo.
echo [RunBuildKonohaVillage02AndLevel_fixed] Finished.
pause
endlocal
goto :eof

:error_exit
echo.
echo [RunBuildKonohaVillage02AndLevel_fixed] Aborted.
pause
endlocal
exit /b 1
