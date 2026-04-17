@echo off
setlocal EnableExtensions
cd /d "%~dp0"

if not defined KNV03_MESH_SRC set "KNV03_MESH_SRC=D:\NarutoExports\Game\Environments\MapAssets\KonohaVillage03\Meshes"
if not defined KNV03_MI_ROOT set "KNV03_MI_ROOT=D:\NarutoExports\Game\Environments\MapAssets\KonohaVillage03\Materials"
if not defined KNV03_TEXTURE_ROOT set "KNV03_TEXTURE_ROOT=D:\NarutoExports\Game\Environments\LobbyMapAssets\LM_KonohaVillage_BORUTO\Textures"
if not defined KNV03_LEVEL_MAP_ROOT set "KNV03_LEVEL_MAP_ROOT=D:\NarutoExports\MapJSON\KonohaVillage03"

set "KNV03_MESH_DST=..\..\StaticMesh\KonohaVillage03\Meshes"
set "KNV03_TEX_DST=..\..\Textures\FModel\KonohaVillage03"
set "KNV03_MATINST_DST=..\..\Materials\FModel\KonohaVillage03"
set "KNV03_GUID_MAP=..\json\KonohaVillage03_mesh_guid_map.json"
set "KNV03_LEVEL_OUT=..\json\Levels"

if not defined ASSIMP_TOOL set "ASSIMP_TOOL=%~dp0..\..\..\..\..\AssimpTool\Bin\AssimpTool.exe"

echo [RunBuildKonohaVillage03AndLevel_fixed] Working directory: %CD%
echo   KNV03_MESH_SRC=%KNV03_MESH_SRC%
echo   KNV03_MI_ROOT=%KNV03_MI_ROOT%
echo   KNV03_TEXTURE_ROOT=%KNV03_TEXTURE_ROOT%
echo   KNV03_LEVEL_MAP_ROOT=%KNV03_LEVEL_MAP_ROOT%
echo   KNV03_MESH_DST=%KNV03_MESH_DST%
echo   KNV03_TEX_DST=%KNV03_TEX_DST%
echo   KNV03_MATINST_DST=%KNV03_MATINST_DST%
echo   KNV03_GUID_MAP=%KNV03_GUID_MAP%
echo   KNV03_LEVEL_OUT=%KNV03_LEVEL_OUT%
echo   ASSIMP_TOOL=%ASSIMP_TOOL%

python "%~dp0BuildExamStadiumPipeline.py" --assimp-tool "%ASSIMP_TOOL%" --mesh-src "%KNV03_MESH_SRC%" --mesh-dst "%KNV03_MESH_DST%" --mi-root "%KNV03_MI_ROOT%" --texture-root "%KNV03_TEXTURE_ROOT%" --copy-textures-to "%KNV03_TEX_DST%" --matinst-root "%KNV03_MATINST_DST%" --level-map-root "%KNV03_LEVEL_MAP_ROOT%" --guid-map-out "%KNV03_GUID_MAP%" --level-out-dir "%KNV03_LEVEL_OUT%" %*

echo.
echo [RunBuildKonohaVillage03AndLevel_fixed] Finished.
pause
endlocal
