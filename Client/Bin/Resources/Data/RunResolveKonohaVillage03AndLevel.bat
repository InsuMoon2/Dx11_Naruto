@echo off
cd /d "%~dp0"

echo [RunResolveKonohaVillage03AndLevel] Working directory: %CD%
python ResolveFModelMaterials.py ^
  --mode all ^
  --run-level-convert ^
  --mesh-root "D:\GitDesktop\Dx11_Naruto\Client\Bin\Resources\StaticMesh\KonohaVillage03\Meshes" ^
  --mi-root "D:\NarutoExports\Game\Environments\MapAssets\KonohaVillage03\Materials" ^
  --texture-root "C:\Users\Moon In Su\Desktop\FModel\Output\Exports\NARUTO\Content\Environments\LobbyMapAssets\LM_KonohaVillage_BORUTO\Textures" ^
  --copy-textures-to "D:\GitDesktop\Dx11_Naruto\Client\Bin\Resources\Textures\FModel\KonohaVillage03" ^
  --matinst-root "D:\GitDesktop\Dx11_Naruto\Client\Bin\Resources\Materials\FModel\KonohaVillage03" ^
  --level-map-root "D:\NarutoExports\MapJSON\KonohaVillage03" ^
  --level-guid-map "D:\GitDesktop\Dx11_Naruto\Client\Bin\Resources\Data\json\KonohaVillage03_mesh_guid_map.json" ^
  --level-out-dir "D:\GitDesktop\Dx11_Naruto\Client\Bin\Resources\Data\json\Levels" ^
  %*

echo.
echo [RunResolveKonohaVillage03AndLevel] Finished.
pause
