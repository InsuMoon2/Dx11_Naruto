@echo off
setlocal EnableExtensions
cd /d "%~dp0"

REM ExamStadium input paths
if not defined EXAM_MESH_SRC set "EXAM_MESH_SRC=D:\NarutoExports\Game\Environments\TrainingMapAssets\TM_ChuninExamArena\Meshes"
if not defined EXAM_MI_ROOT set "EXAM_MI_ROOT=D:\NarutoExports\Game\Environments\TrainingMapAssets\TM_ChuninExamArena\Materials"
if not defined EXAM_TEXTURE_ROOT set "EXAM_TEXTURE_ROOT=C:\Users\Moon In Su\Desktop\FModel\Output\Exports\NARUTO\Content\Environments\TrainingMapAssets\TM_ChuninExamArena\Textures"
if not defined EXAM_LEVEL_MAP_ROOT set "EXAM_LEVEL_MAP_ROOT=D:\NarutoExports\MapJSON\ExamStadium"

REM Output paths are always fixed. Do not inherit them from outer env vars.
set "EXAM_MESH_DST=..\..\StaticMesh\ExamStadium\Meshes"
set "EXAM_TEX_DST=..\..\Textures\FModel\ExamStadium"
set "EXAM_MATINST_DST=..\..\Materials\FModel\ExamStadium"
set "EXAM_GUID_MAP=..\json\ExamStadium_mesh_guid_map.json"
set "EXAM_LEVEL_OUT=..\json\Levels"

REM AssimpTool path from MapTools
if not defined ASSIMP_TOOL set "ASSIMP_TOOL=..\..\..\..\..\AssimpTool\Bin\AssimpTool.exe"

echo [RunBuildExamStadiumAndLevel] Working directory: %CD%
echo   EXAM_MESH_SRC       = %EXAM_MESH_SRC%
echo   EXAM_MI_ROOT        = %EXAM_MI_ROOT%
echo   EXAM_TEXTURE_ROOT   = %EXAM_TEXTURE_ROOT%
echo   EXAM_LEVEL_MAP_ROOT = %EXAM_LEVEL_MAP_ROOT%
echo   EXAM_MESH_DST       = %EXAM_MESH_DST%
echo   EXAM_TEX_DST        = %EXAM_TEX_DST%
echo   EXAM_MATINST_DST    = %EXAM_MATINST_DST%
echo   EXAM_GUID_MAP       = %EXAM_GUID_MAP%
echo   EXAM_LEVEL_OUT      = %EXAM_LEVEL_OUT%
echo   ASSIMP_TOOL         = %ASSIMP_TOOL%

python BuildExamStadiumPipeline.py ^
  --assimp-tool "%ASSIMP_TOOL%" ^
  --mesh-src "%EXAM_MESH_SRC%" ^
  --mesh-dst "%EXAM_MESH_DST%" ^
  --mi-root "%EXAM_MI_ROOT%" ^
  --texture-root "%EXAM_TEXTURE_ROOT%" ^
  --copy-textures-to "%EXAM_TEX_DST%" ^
  --matinst-root "%EXAM_MATINST_DST%" ^
  --level-map-root "%EXAM_LEVEL_MAP_ROOT%" ^
  --guid-map-out "%EXAM_GUID_MAP%" ^
  --level-out-dir "%EXAM_LEVEL_OUT%" ^
  %*

echo.
echo [RunBuildExamStadiumAndLevel] Finished.
pause
endlocal
