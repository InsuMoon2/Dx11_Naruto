@echo off
REM 머티리얼 파이프라인 실행 후 레벨 변환까지 이어서 수행
cd /d "%~dp0"

echo [RunResolveMaterialsAndLevel] Working directory: %CD%
REM [유지] ResolveFModelMaterials.py 는 같은 MapTools 폴더에 있으므로 파일명만으로 호출 가능
python ResolveFModelMaterials.py --mode all --run-level-convert %*
if errorlevel 1 goto :end

:end
echo.
echo [RunResolveMaterialsAndLevel] Finished.
pause
