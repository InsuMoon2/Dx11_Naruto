@echo off
REM ResolveFModelMaterials 전체 파이프라인만 실행
cd /d "%~dp0"

echo [RunResolveMaterials] Working directory: %CD%
REM [유지] ResolveFModelMaterials.py 는 같은 MapTools 폴더에 있으므로 파일명만으로 호출 가능
python ResolveFModelMaterials.py --mode all %*

echo.
echo [RunResolveMaterials] Finished.
pause
