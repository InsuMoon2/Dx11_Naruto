@echo off
cd /d "%~dp0"

REM ============================================================
REM  FModel 추출 경로 (PC마다 다름 - 여기만 수정하면 됨)
REM ============================================================
if not defined FMODEL_MI_ROOT      set "FMODEL_MI_ROOT=D:\NarutoExports\Game\Environments\MapAssets\KonohaVillage03\Materials"
if not defined FMODEL_TEXTURE_ROOT set "FMODEL_TEXTURE_ROOT=C:\Users\Moon In Su\Desktop\FModel\Output\Exports\NARUTO\Content\Environments\LobbyMapAssets\LM_KonohaVillage_BORUTO\Textures"
if not defined FMODEL_LEVEL_ROOT   set "FMODEL_LEVEL_ROOT=D:\NarutoExports\MapJSON\KonohaVillage03"

echo [RunResolveKonohaVillage03AndLevel] Working directory: %CD%
echo   FMODEL_MI_ROOT      = %FMODEL_MI_ROOT%
echo   FMODEL_TEXTURE_ROOT = %FMODEL_TEXTURE_ROOT%
echo   FMODEL_LEVEL_ROOT   = %FMODEL_LEVEL_ROOT%

python ResolveFModelMaterials.py ^
  --mode all ^
  --run-level-convert ^
  --mesh-root "..\StaticMesh\KonohaVillage03\Meshes" ^
  --mi-root "%FMODEL_MI_ROOT%" ^
  --texture-root "%FMODEL_TEXTURE_ROOT%" ^
  --copy-textures-to "..\Textures\FModel\KonohaVillage03" ^
  --matinst-root "..\Materials\FModel\KonohaVillage03" ^
  --level-map-root "%FMODEL_LEVEL_ROOT%" ^
  --level-guid-map "json\KonohaVillage03_mesh_guid_map.json" ^
  --level-out-dir "json\Levels" ^
  %*

echo.
echo [RunResolveKonohaVillage03AndLevel] Finished.
pause
