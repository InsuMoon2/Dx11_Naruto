@echo off
setlocal EnableExtensions
cd /d "%~dp0"

REM Restore default input paths even when outer environment variables are blank.
if "%EXAM_MESH_SRC%"=="" set "EXAM_MESH_SRC=D:\NarutoExports\Game\Environments\TrainingMapAssets\TM_ChuninExamArena\Meshes"
if "%EXAM_MI_ROOT%"=="" set "EXAM_MI_ROOT=D:\NarutoExports\Game\Environments\TrainingMapAssets\TM_ChuninExamArena\Materials"
if "%EXAM_TEXTURE_ROOT%"=="" set "EXAM_TEXTURE_ROOT=D:\NarutoExports\Game\Environments\TrainingMapAssets\TM_ChuninExamArena\Textures"
if "%EXAM_EXTRA_TEXTURE_ROOT_1%"=="" set "EXAM_EXTRA_TEXTURE_ROOT_1=D:\NarutoExports\Game\Environments\LobbyMapAssets\LM_KonohaVillage_BORUTO\Textures"
if "%EXAM_EXTRA_TEXTURE_ROOT_2%"=="" set "EXAM_EXTRA_TEXTURE_ROOT_2=D:\NarutoExports\Game\Environments\MapAssets\KonohaForest\Textures"
if "%EXAM_LEVEL_MAP_ROOT%"=="" set "EXAM_LEVEL_MAP_ROOT=D:\NarutoExports\MapJSON\ExamStadium"

set "EXAM_MESH_DST=..\..\StaticMesh\ExamStadium\Meshes"
set "EXAM_TEX_DST=..\..\Textures\FModel\ExamStadium"
set "EXAM_MATINST_DST=..\..\Materials\FModel\ExamStadium"
set "EXAM_GUID_MAP=..\json\ExamStadium_mesh_guid_map.json"
set "EXAM_LEVEL_OUT=..\json\Levels"

if not defined ASSIMP_TOOL set "ASSIMP_TOOL=%~dp0..\..\..\..\..\AssimpTool\Bin\AssimpTool.exe"

REM Fail fast when a required input path is missing.
if not exist "%EXAM_MESH_SRC%" (
  echo [ERROR] EXAM_MESH_SRC not found: %EXAM_MESH_SRC%
  goto :error_exit
)

if not exist "%EXAM_MI_ROOT%" (
  echo [ERROR] EXAM_MI_ROOT not found: %EXAM_MI_ROOT%
  goto :error_exit
)

if not exist "%EXAM_TEXTURE_ROOT%" (
  echo [ERROR] EXAM_TEXTURE_ROOT not found: %EXAM_TEXTURE_ROOT%
  goto :error_exit
)

if not exist "%EXAM_EXTRA_TEXTURE_ROOT_1%" (
  echo [ERROR] EXAM_EXTRA_TEXTURE_ROOT_1 not found: %EXAM_EXTRA_TEXTURE_ROOT_1%
  goto :error_exit
)

if not exist "%EXAM_EXTRA_TEXTURE_ROOT_2%" (
  echo [ERROR] EXAM_EXTRA_TEXTURE_ROOT_2 not found: %EXAM_EXTRA_TEXTURE_ROOT_2%
  goto :error_exit
)

if not exist "%EXAM_LEVEL_MAP_ROOT%" (
  echo [ERROR] EXAM_LEVEL_MAP_ROOT not found: %EXAM_LEVEL_MAP_ROOT%
  goto :error_exit
)

echo [RunBuildExamStadiumAndLevel] Working directory: %CD%
echo   EXAM_MESH_SRC=%EXAM_MESH_SRC%
echo   EXAM_MI_ROOT=%EXAM_MI_ROOT%
echo   EXAM_TEXTURE_ROOT=%EXAM_TEXTURE_ROOT%
echo   EXAM_EXTRA_TEXTURE_ROOT_1=%EXAM_EXTRA_TEXTURE_ROOT_1%
echo   EXAM_EXTRA_TEXTURE_ROOT_2=%EXAM_EXTRA_TEXTURE_ROOT_2%
echo   EXAM_LEVEL_MAP_ROOT=%EXAM_LEVEL_MAP_ROOT%
echo   EXAM_MESH_DST=%EXAM_MESH_DST%
echo   EXAM_TEX_DST=%EXAM_TEX_DST%
echo   EXAM_MATINST_DST=%EXAM_MATINST_DST%
echo   EXAM_GUID_MAP=%EXAM_GUID_MAP%
echo   EXAM_LEVEL_OUT=%EXAM_LEVEL_OUT%
echo   ASSIMP_TOOL=%ASSIMP_TOOL%

python "%~dp0BuildExamStadiumPipeline.py" ^
  --assimp-tool "%ASSIMP_TOOL%" ^
  --mesh-src "%EXAM_MESH_SRC%" ^
  --mesh-dst "%EXAM_MESH_DST%" ^
  --mi-root "%EXAM_MI_ROOT%" ^
  --texture-root "%EXAM_TEXTURE_ROOT%" ^
  --extra-texture-root "%EXAM_EXTRA_TEXTURE_ROOT_1%" ^
  --extra-texture-root "%EXAM_EXTRA_TEXTURE_ROOT_2%" ^
  --copy-textures-to "%EXAM_TEX_DST%" ^
  --matinst-root "%EXAM_MATINST_DST%" ^
  --level-map-root "%EXAM_LEVEL_MAP_ROOT%" ^
  --guid-map-out "%EXAM_GUID_MAP%" ^
  --level-out-dir "%EXAM_LEVEL_OUT%" ^
  %*

if errorlevel 1 goto :error_exit

echo.
echo [RunBuildExamStadiumAndLevel] Finished.
pause
endlocal
goto :eof

:error_exit
echo.
echo [RunBuildExamStadiumAndLevel] Aborted.
pause
endlocal
exit /b 1
