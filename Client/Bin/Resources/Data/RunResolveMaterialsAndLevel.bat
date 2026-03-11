@echo off
REM 머티리얼 파이프라인 실행 후 레벨 변환까지 이어서 수행
cd /d "%~dp0"

echo [RunResolveMaterialsAndLevel] Working directory: %CD%
python ResolveFModelMaterials.py --mode all --run-level-convert %*
if errorlevel 1 goto :end

:end
echo.
echo [RunResolveMaterialsAndLevel] Finished.
pause
