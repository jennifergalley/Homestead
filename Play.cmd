@echo off
setlocal
rem Launch through the Homestead-named hard link so Discord doesn't report the game as Outpost Zero.
set "Game=%~dp0Build\Windows\SurvivalGame\Binaries\Win64\JennysHomesteadGame.exe"
if not exist "%Game%" (
    echo No packaged game was found.
    echo Build it with Scripts\Build-Game.ps1 -Package first.
    pause
    exit /b 1
)
start "" "%Game%" %*
