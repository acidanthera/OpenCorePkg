@echo off
setlocal

set "SCRIPT_DIR=%~dp0"

where py >nul 2>nul
if %ERRORLEVEL% EQU 0 (
  py -3 "%SCRIPT_DIR%macrecovery.py" %*
  exit /b %ERRORLEVEL%
)

where python >nul 2>nul
if %ERRORLEVEL% EQU 0 (
  python "%SCRIPT_DIR%macrecovery.py" %*
  exit /b %ERRORLEVEL%
)

echo Python launcher 'py' or 'python' was not found in PATH. 1>&2
echo Install Python 3 from https://www.python.org/downloads/windows/ and try again. 1>&2
exit /b 1
