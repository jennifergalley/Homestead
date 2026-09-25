@echo off
setlocal
where pwsh >nul 2>nul
if errorlevel 1 (
    echo The character-first trial requires PowerShell 7 ^(pwsh^). See docs\setup.md.
    pause
    exit /b 1
)
pwsh -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\Start-CharacterTrial.ps1"
if errorlevel 1 (
    echo The experimental character trial did not start or exited with an error.
    pause
    exit /b 1
)
