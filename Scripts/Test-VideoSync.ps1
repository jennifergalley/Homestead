[CmdletBinding()]
param([string]$EngineRoot, [switch]$Packaged, [string]$PackageDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [ValidateSet(1280,3840)][int]$Width=1280, [ValidateSet(720,2160)][int]$Height=720)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$output=[IO.Path]::GetFullPath($OutputDirectory,$root)
if(Test-Path -LiteralPath $output){throw 'Use a fresh video-sync fixture directory.'}
if(($Width -eq 1280) -ne ($Height -eq 720)){throw 'Use matching720p or4K dimensions.'}
if($Packaged) {
    $working=& (Join-Path $PSScriptRoot 'Resolve-PackageDirectory.ps1') -PackageDirectory $PackageDirectory
    $exe=Join-Path $working 'SurvivalGame\Binaries\Win64\SurvivalGame.exe'
    $prefix=''
} else {
    $engine=& (Join-Path $PSScriptRoot 'Resolve-Engine.ps1') -EngineRoot $EngineRoot
    $working=$root
    $exe=Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor.exe'
    $offlineArguments=(& (Join-Path $PSScriptRoot 'Get-UnrealOfflineArguments.ps1')) -join ' '
    $prefix="`"$(Join-Path $root 'SurvivalGame.uproject')`" /Game/SurvivalGame/Maps/Homestead -game $offlineArguments "
}
& (Join-Path $PSScriptRoot 'Set-EngineEnvironment.ps1')
$null=New-Item -ItemType Directory -Path (Join-Path $output 'Graphics')
$ini=Join-Path $output 'Graphics\GameUserSettings.ini'
@'
[/Script/Engine.GameUserSettings]
bUseVSync=False
bUseDynamicResolution=False
ResolutionSizeX=1600
ResolutionSizeY=900
LastUserConfirmedResolutionSizeX=1600
LastUserConfirmedResolutionSizeY=900
FullscreenMode=2
LastConfirmedFullscreenMode=2
PreferredFullscreenMode=1
FrameRateLimit=57.000000
Version=5

[ScalabilityGroups]
sg.ResolutionQuality=73
sg.ViewDistanceQuality=2
'@ | Set-Content -LiteralPath $ini
Copy-Item -LiteralPath $ini -Destination (Join-Path $output 'initial-fixture.ini')
$runs=@()
foreach($phase in @('write-on','read-on-write-off','read-off','override')) {
    $runRoot=$output
    if($phase -eq 'override') {
        $runRoot=Join-Path $output 'override'
        $null=New-Item -ItemType Directory -Path (Join-Path $runRoot 'Graphics')
        Copy-Item -LiteralPath $ini -Destination (Join-Path $runRoot 'Graphics\GameUserSettings.ini')
    }
    $config=Join-Path $runRoot 'Graphics\GameUserSettings.ini'
    $log=Join-Path $runRoot "$phase.log"
    $commands=if($phase -eq 'override'){'r.ScreenPercentage 85,r.VSync 1'}else{'r.ScreenPercentage 85'}
    $arguments=$prefix+"-HomesteadSmokeTest -HomesteadVideoSyncTest -HomesteadVideoSyncPhase=$phase -HomesteadTestOutput=`"$runRoot`" -GameUserSettingsINI=`"$config`" -UserDir=`"$(Join-Path $runRoot 'EngineUser')`" -unattended -RenderOffscreen -windowed -ForceRes -ResX=$Width -ResY=$Height -ExecCmds=`"$commands`" -nosound -nosplash -abslog=`"$log`""
    $process=Start-Process -FilePath $exe -WorkingDirectory $working -ArgumentList $arguments -PassThru
    try {
        if(-not $process.WaitForExit(300000)){throw "Video-sync fixture timed out: $phase PID$($process.Id)"}
        $resultPath=Join-Path $runRoot 'smoke-result.txt'
        $result=Get-Content -LiteralPath $resultPath -Raw
        if($process.ExitCode -ne 0 -or $result -notmatch '(?m)^SUCCESS '){throw "Video-sync fixture failed: $phase. See $runRoot"}
        $baseline=Get-Item -LiteralPath (Join-Path $runRoot "$phase-other-preferences.txt")
        if($baseline.Length -eq 0){throw 'Native unrelated-preference baseline evidence is missing or empty.'}
        $actual=([regex]::Match($result,'(?m)^GRAPHICS_CONFIG=([^\r\n]+)')).Groups[1].Value.Replace('/','\')
        if([IO.Path]::GetFullPath($actual) -ne $config){throw 'Engine graphics write destination was not the synthetic file.'}
        if(@(Get-ChildItem -Path (Join-Path $runRoot 'SmokeSave') -File -ErrorAction SilentlyContinue).Count){throw 'Video toggle wrote a game save.'}
        $expected=if($phase -in @('write-on','override')){'True'}else{'False'}
        $stored=[regex]::Match((Get-Content -LiteralPath $config -Raw),'(?im)^bUseVSync=(True|False)\s*$')
        # UE removes values equal to project defaults on shutdown; the next native process verifies the effective choice.
        if(-not $stored.Success) {
            $stored=[regex]::Match((Get-Content (Join-Path $root 'Config\DefaultGameUserSettings.ini') -Raw),'(?im)^bUseVSync=(True|False)\s*$')
        }
        if(-not $stored.Success -or $stored.Groups[1].Value -ne $expected) {
            throw "Saved preference disagrees after $phase"
        }
        Copy-Item -LiteralPath $resultPath -Destination (Join-Path $runRoot "$phase-result.txt")
        Copy-Item -LiteralPath $config -Destination (Join-Path $runRoot "$phase-saved.ini")
        $runs+=@{phase=$phase;pid=$process.Id;checks=[regex]::Matches($result,'(?m)^PASS ').Count;config=$actual;arguments=$arguments}
    } finally {
        if(-not $process.HasExited){Stop-Process -Id $process.Id}
        $process.Dispose()
        if(Test-Path -LiteralPath $config){(Get-Item -LiteralPath $config).IsReadOnly=$false}
    }
}
Add-Type -AssemblyName System.Drawing
foreach($name in @('sync-off-gamepad','sync-on-gamepad','sync-off-keyboard','sync-on-keyboard','override\sync-override')) {
    $image=[Drawing.Image]::FromFile((Join-Path $output "$name.png"))
    try {if($image.Width -ne $Width -or $image.Height -ne $Height){throw 'Incorrect rendered dimensions.'}}finally{$image.Dispose()}
    if((Get-Content (Join-Path $output "$name.frame.txt") -Raw) -notmatch 'book_text_fits=1'){throw 'Native Settings text overflowed.'}
}
[ordered]@{status='passed';executableSha256=(Get-FileHash $exe).Hash;width=$Width;height=$Height;runs=$runs
    mainFixtureFinalPreference='Off';overrideFixtureFinalPreference='On, separate synthetic file only'
    userConfig='Explicit GameUserSettingsINI plus synthetic UserDir; actual runtime destination asserted before toggle.'
    limits='Mapped engine input and offscreen rendered Settings; no desktop input, physical scanout, VRR or tearing-improvement proof.'
} | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $output 'video-sync-result.json')
Write-Output "Video-sync fixtures passed: $output"
