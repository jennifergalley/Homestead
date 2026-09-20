@echo off
setlocal
where pwsh >nul 2>nul
if errorlevel 1 (
    echo Preview requires PowerShell 7 ^(pwsh^). See docs\setup.md.
    pause
    exit /b 1
)
pwsh -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\Start-Preview.ps1"
if errorlevel 1 (
    echo Preview was not started or exited with an error. The original Play.cmd is unchanged.
    pause
    exit /b 1
)
