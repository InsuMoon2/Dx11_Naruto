@echo off
setlocal EnableExtensions
cd /d "%~dp0"

if not defined KNV02_MESH_SRC set "KNV02_MESH_SRC=D:\NarutoExports\Game\Environments\MapAssets\KonohaVillage02\Meshes"
if not defined KNV02_MI_ROOT set "KNV02_MI_ROOT=D:\NarutoExports\Game\Environments\MapAssets\KonohaVillage02\Materials"
if not defined KNV02_TEXTURE_ROOT set "KNV02_TEXTURE_ROOT=D:\NarutoExports\Game\Environments\MapAssets\KonohaVillage02\Textures"
if not defined KNV02_LEVEL_MAP_ROOT set "KNV02_LEVEL_MAP_ROOT=D:\NarutoExports\MapJSON\KonohaVillage02"

set "KNV02_MESH_DST=..\..\StaticMesh\KonohaVillage02\Meshes"
set "KNV02_TEX_DST=..\..\Textures\FModel\KonohaVillage02"
set "KNV02_MATINST_DST=..\..\Materials\FModel\KonohaVillage02"
set "KNV02_GUID_MAP=..\json\KonohaVillage02_mesh_guid_map.json"
set "KNV02_LEVEL_OUT=..\json\Levels"

if not defined ASSIMP_TOOL set "ASSIMP_TOOL=%~dp0..\..\..\..\..\AssimpTool\Bin\AssimpTool.exe"

echo [RunBuildKonohaVillage02AndLevel_fixed] Working directory: %CD%
echo   KNV02_MESH_SRC=%KNV02_MESH_SRC%
echo   KNV02_MI_ROOT=%KNV02_MI_ROOT%
echo   KNV02_TEXTURE_ROOT=%KNV02_TEXTURE_ROOT%
echo   KNV02_LEVEL_MAP_ROOT=%KNV02_LEVEL_MAP_ROOT%
echo   KNV02_MESH_DST=%KNV02_MESH_DST%
echo   KNV02_TEX_DST=%KNV02_TEX_DST%
echo   KNV02_MATINST_DST=%KNV02_MATINST_DST%
echo   KNV02_GUID_MAP=%KNV02_GUID_MAP%
echo   KNV02_LEVEL_OUT=%KNV02_LEVEL_OUT%
echo   ASSIMP_TOOL=%ASSIMP_TOOL%

python "%~dp0BuildExamStadiumPipeline.py" ^
  --assimp-tool "%ASSIMP_TOOL%" ^
  --mesh-src "%KNV02_MESH_SRC%" ^
  --mesh-dst "%KNV02_MESH_DST%" ^
  --mi-root "%KNV02_MI_ROOT%" ^
  --texture-root "%KNV02_TEXTURE_ROOT%" ^
  --copy-textures-to "%KNV02_TEX_DST%" ^
  --matinst-root "%KNV02_MATINST_DST%" ^
  --level-map-root "%KNV02_LEVEL_MAP_ROOT%" ^
  --guid-map-out "%KNV02_GUID_MAP%" ^
  --level-out-dir "%KNV02_LEVEL_OUT%" ^
  %*

echo.
echo [RunBuildKonohaVillage02AndLevel_fixed] Finished.
pause
endlocal
