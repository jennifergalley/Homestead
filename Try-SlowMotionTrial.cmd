@echo off
setlocal
where pwsh >nul 2>nul
if errorlevel 1 (
    echo The motion trial requires PowerShell 7 ^(pwsh^). See docs\setup.md.
    pause
    exit /b 1
)
pwsh -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\Start-CharacterTrial.ps1" -Vitruvian -CMUSlowBlend
if errorlevel 1 (
    echo The experimental slow/full motion trial did not start or exited with an error.
    pause
    exit /b 1
)
