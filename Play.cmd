@echo off
setlocal
set "Game=%~dp0Build\Windows\SurvivalGame.exe"
if not exist "%Game%" (
    echo No packaged game was found.
    echo Build it with Scripts\Build-Game.ps1 -Package first.
    pause
    exit /b 1
)
start "" "%Game%"
