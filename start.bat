@echo off
rem XCPC Helper launcher: double-click to start.
rem Prefers the bundled uv.exe (portable package); falls back to uv on PATH
rem (source checkout). Keep this file pure ASCII to avoid cmd encoding issues.
cd /d "%~dp0"

if exist "%~dp0uv.exe" (set "UV=%~dp0uv.exe") else (set "UV=uv")

rem The install location may be on a different drive than uv's cache (C:),
rem where hardlinks are unsupported. Use copy mode to avoid a noisy warning.
set UV_LINK_MODE=copy

rem Source checkout has no frontend/dist in git. Build it once when missing.
rem Release zips already ship frontend/dist and do not include package.json.
if not exist "%~dp0frontend\dist\index.html" (
  if not exist "%~dp0frontend\package.json" (
    echo Frontend files are missing. Use a Release zip, or clone the full repo.
    pause
    exit /b 1
  )
  where npm >nul 2>&1
  if errorlevel 1 (
    echo npm is not on PATH. Install Node.js, or run scripts\dev.ps1 first.
    pause
    exit /b 1
  )
  echo Frontend build not found. Building ^(needs Node.js, first time may take a few minutes^)...
  echo.
  pushd "%~dp0frontend"
  if not exist "node_modules\" (
    call npm ci
    if errorlevel 1 (
      popd
      echo npm ci failed.
      pause
      exit /b 1
    )
  )
  call npm run build
  if errorlevel 1 (
    popd
    echo npm run build failed.
    pause
    exit /b 1
  )
  popd
  echo.
)

echo ============================================================
echo   XCPC Helper
echo.
echo   First run will download Python and dependencies
echo   automatically, which takes a few minutes (needs network).
echo   Subsequent starts are instant and work offline.
echo.
echo   The browser will open http://127.0.0.1:8000 when ready.
echo   Keep this window open while using the app.
echo ============================================================
echo.

rem Poll until the API is up, then open the default browser.
rem First run (Python download + startup sync) can take several minutes.
start "" /b powershell -NoProfile -WindowStyle Hidden -Command "for ($i=0; $i -lt 600; $i++) { try { Invoke-WebRequest -UseBasicParsing 'http://127.0.0.1:8000/api/diagnostics' -TimeoutSec 2 | Out-Null; Start-Process 'http://127.0.0.1:8000'; exit 0 } catch { Start-Sleep -Seconds 1 } }"

"%UV%" run --directory backend --frozen uvicorn --app-dir src main:app --host 127.0.0.1 --port 8000
pause
