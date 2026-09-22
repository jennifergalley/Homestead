[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$PackageDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [switch]$Baseline
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$output=[IO.Path]::GetFullPath($OutputDirectory,$root)
if(Test-Path -LiteralPath $output){throw 'Use a fresh hotkey fixture output.'}
$details=& (Join-Path $PSScriptRoot 'Resolve-PackageDirectory.ps1') -PackageDirectory $PackageDirectory -Details
$package=$details.packageDirectory
$exe=$details.executable
& (Join-Path $PSScriptRoot 'Set-EngineEnvironment.ps1')
$null=New-Item -ItemType Directory -Path (Join-Path $output 'Graphics')
$config=Join-Path $output 'Graphics\GameUserSettings.ini'
Copy-Item -LiteralPath (Join-Path $root 'Config\DefaultGameUserSettings.ini') -Destination $config
$phases=if($Baseline){@('baseline')}else{@('write','reload')}
$results=@()
foreach($phase in $phases){
    $log=Join-Path $output "$phase.log"
    $arguments="-HomesteadSmokeTest -HomesteadHotkeyTest -HomesteadHotkeyPhase=$phase -HomesteadTestOutput=`"$output`" -GameUserSettingsINI=`"$config`" -UserDir=`"$(Join-Path $output 'EngineUser')`" -unattended -RenderOffscreen -windowed -ForceRes -ResX=1280 -ResY=720 -ExecCmds=`"r.ScreenPercentage 100`" -nosound -nosplash -abslog=`"$log`""
    $process=Start-Process -FilePath $exe -WorkingDirectory $package -ArgumentList $arguments -PassThru
    Write-Host "Owned hotkey fixture $phase PID$($process.Id):$output"
    try{
        if(-not $process.WaitForExit(300000)){throw "Hotkey fixture timeout:$phase"}
        $report=Get-Content (Join-Path $output 'smoke-result.txt') -Raw
        if($process.ExitCode -ne 0 -or $report -notmatch '(?m)^SUCCESS '){throw "Hotkey fixture failed:$phase. See $output"}
        Copy-Item (Join-Path $output 'smoke-result.txt') (Join-Path $output "$phase-result.txt")
        $rows=@(Get-Content (Join-Path $output "hotkey-$phase.jsonl") | ForEach-Object {$_ | ConvertFrom-Json})
        if(-not $rows.Count){throw 'Missing actual renderer/action snapshots'}
        $shots=([regex]::Match($report,'(?m)^SCREENSHOT_DIRECTORY=([^\r\n]+)')).Groups[1].Value
        if(-not ([IO.Path]::GetFullPath($shots)).StartsWith($output+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){
            throw 'Screenshot destination escaped fixture'
        }
        $count=@(Get-ChildItem -LiteralPath $shots -Filter '*.png' -File -Recurse -ErrorAction SilentlyContinue).Count
        if($count -ne $(if($Baseline){2}else{0})){throw "Unexpected screenshot files:$count"}
        $saveDirectory=([regex]::Match($report,'(?m)^SAVE_DIRECTORY=([^\r\n]+)')).Groups[1].Value
        $profile=([regex]::Match($report,'(?m)^PROFILE=([^\r\n]+)')).Groups[1].Value
        $results+=@{phase=$phase;checks=[regex]::Matches($report,'(?m)^PASS ').Count;pid=$process.Id
            screenshots=$count;saveDirectory=$saveDirectory;profile=$profile;arguments=$arguments}
    }finally{
        if(-not $process.HasExited){Stop-Process -Id $process.Id}
        $process.Dispose()
    }
}
if(-not $Baseline){
    if($results[0].profile -notmatch '^hotkey-[a-f0-9]{12}$' -or $results[0].profile -ne $results[1].profile -or
        $results[0].saveDirectory -ne $results[1].saveDirectory){throw 'Profile relaunch did not use the same synthetic namespace'}
    $copy=Join-Path $output 'ProfileSaveCopies'
    $null=New-Item -ItemType Directory -Path $copy
    Copy-Item -LiteralPath (Get-ChildItem -LiteralPath $results[0].saveDirectory -File).FullName -Destination $copy
    $before=@{}
    foreach($file in Get-ChildItem -LiteralPath $results[0].saveDirectory -File){$before[$file.FullName]=(Get-FileHash $file.FullName).Hash}
    $expected=$rows[-1]
    $log=Join-Path $output 'normal-preview-relaunch.log'
    $arguments="-HomesteadPreviewProfile=$($results[0].profile) -HomesteadSaveAudit -GameUserSettingsINI=`"$config`" -UserDir=`"$(Join-Path $output 'EngineUser')`" -RenderOffscreen -windowed -ForceRes -ResX=1280 -ResY=720 -nosound -nosplash -abslog=`"$log`""
    $process=Start-Process -FilePath $exe -WorkingDirectory $package -ArgumentList $arguments -PassThru
    Write-Host "Owned normal saved-preview relaunch PID$($process.Id):$log"
    try{
        $ready=$false
        $limit=[DateTimeOffset]::UtcNow.AddSeconds(90)
        while([DateTimeOffset]::UtcNow -lt $limit -and -not $process.HasExited){
            if(Test-Path $log){
                $text=Get-Content $log -Raw
                if($text -match 'SAVE_LOAD_AUDIT world=([a-fA-F0-9]+) simulation_md5=([a-fA-F0-9]+) look=([0-9,]+) view_mode=(\d+) shader_complexity=(\d+)'){
                    if($Matches[1] -ne $expected.world -or $Matches[2] -ne $expected.simulationMd5 -or
                        $Matches[3] -ne $expected.look -or $Matches[4] -ne '3' -or $Matches[5] -ne '0'){throw 'Normal preview auto-load state/renderer differs'}
                    if($text -notmatch 'automation_input=0 smoke_actor=0 visual_actor=0'){throw 'Normal preview gained an automation actor/input policy'}
                    $ready=$true;break
                }
            }
            Start-Sleep -Milliseconds 500
        }
        if(-not $ready){throw 'Normal saved-preview startup did not produce read-only load proof'}
        Start-Sleep -Seconds 3
        if($process.HasExited){throw 'Normal preview exited automatically'}
        $normalPreview=@{pid=$process.Id;profile=$results[0].profile;loadedExactState=$true;viewMode=3;automatedInput=$false;automaticExit=$false}
    }finally{
        if(-not $process.HasExited){Stop-Process -Id $process.Id; $process.WaitForExit()}
        $process.Dispose()
    }
    foreach($path in $before.Keys){if((Get-FileHash $path).Hash -ne $before[$path]){throw 'Read-only relaunch changed saved profile'}}
}
[ordered]@{status='passed';baseline=[bool]$Baseline;executableSha256=(Get-FileHash $exe).Hash;phases=$results
    normalPreviewRelaunch=$normalPreview
    limits='Opt-in in-engine inputs and observed viewport/ShowFlags, not OS input or physical scanout. Dedicated preview namespace retained and copied; no ambiguous old screenshots removed.'
} | ConvertTo-Json -Depth 7 | Set-Content (Join-Path $output 'hotkey-result.json')
Write-Host "Hotkey fixture passed:$output"
