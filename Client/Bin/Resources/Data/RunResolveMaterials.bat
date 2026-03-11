@echo off
REM ResolveFModelMaterials 전체 파이프라인만 실행
cd /d "%~dp0"

echo [RunResolveMaterials] Working directory: %CD%
python ResolveFModelMaterials.py --mode all %*

echo.
echo [RunResolveMaterials] Finished.
pause
