[CmdletBinding()]
param([string]$EngineRoot, [switch]$Packaged, [string]$PackageDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory, [ValidateRange(30,600)][int]$TimeoutSeconds=180)
$ErrorActionPreference = 'Stop'
$root=Split-Path $PSScriptRoot -Parent
$output=[IO.Path]::GetFullPath($OutputDirectory,$root)
if(Test-Path -LiteralPath $output){throw 'Use a fresh output directory for isolated routing fixtures.'}
$null=New-Item -ItemType Directory -Path $output
if($Packaged) {
    $working=& (Join-Path $PSScriptRoot 'Resolve-PackageDirectory.ps1') -PackageDirectory $PackageDirectory
    $executable=Join-Path $working 'SurvivalGame\Binaries\Win64\SurvivalGame.exe'
    $prefix=''
} else {
    $engine=& (Join-Path $PSScriptRoot 'Resolve-Engine.ps1') -EngineRoot $EngineRoot
    $executable=Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor.exe'
    $working=$root
    $prefix="`"$(Join-Path $root 'SurvivalGame.uproject')`" /Game/SurvivalGame/Maps/Homestead -game "
}
function Invoke-RoutingProcess([string]$Name,[string]$Flags,[int]$ExpectedExit=0) {
    $log=Join-Path $output "$Name.log"
    $arguments=$prefix+"$Flags -HomesteadTestOutput=`"$output`" -nullrhi -nosound -unattended -abslog=`"$log`""
    $process=Start-Process -FilePath $executable -WorkingDirectory $working -ArgumentList $arguments -PassThru
    try {
        if(-not $process.WaitForExit($TimeoutSeconds*1000)){throw "Routing validation timed out: $Name (PID $($process.Id))."}
        if($process.ExitCode -ne $ExpectedExit){throw "Routing process $Name exited $($process.ExitCode), expected $ExpectedExit. See $log"}
    } finally {
        if(-not $process.HasExited){Stop-Process -Id $process.Id}
        $process.Dispose()
    }
    return $log
}
$null=Invoke-RoutingProcess 'routing-write' '-HomesteadSaveRoutingTest'
$write=Get-Content -LiteralPath (Join-Path $output 'routing-write.txt') -Raw
if($write -notmatch '(?m)^SUCCESS '){throw "Routing writes failed. See $output"}
$directories=@([regex]::Matches($write,'(?m)^FIXTURE_[0-3]=([^\r\n]+)') | ForEach-Object { $_.Groups[1].Value.Replace('/','\') })
if($directories.Count -ne 4){throw 'Missing four distinct runtime fixture directories.'}
$before=@{}
foreach($directory in $directories) {
    foreach($file in Get-ChildItem -LiteralPath $directory -File){$before[$file.FullName]=(Get-FileHash -LiteralPath $file.FullName).Hash}
}
$null=Invoke-RoutingProcess 'routing-read' '-HomesteadSaveRoutingTest -HomesteadRoutingReadOnly'
$read=Get-Content -LiteralPath (Join-Path $output 'routing-read.txt') -Raw
if($read -notmatch '(?m)^SUCCESS '){throw "Cross-process routing loads failed. See $output"}
foreach($path in $before.Keys) {
    if((Get-FileHash -LiteralPath $path).Hash -ne $before[$path]){throw "Read-only relaunch changed fixture: $path"}
}
$invalid=@('-HomesteadPreviewProfile','-HomesteadPreviewProfile=','-HomesteadPreviewProfile=../escape',
    '-HomesteadPreviewProfile=a -HomesteadPreviewProfile=b')
for($i=0;$i -lt $invalid.Count;$i++) {
    $log=Invoke-RoutingProcess "invalid-$i" $invalid[$i] 2
    $text=Get-Content -LiteralPath $log -Raw
    if($text -notmatch 'SAVE_ROUTING_REJECTED:' -or $text -match 'SAVE_ROUTING version='){
        throw 'Invalid preview identifier did not fail before initializing any save route.'
    }
}
# Real preview-only startup, with no test/automation/automatic-exit flags.
$probe='input-check-'+[guid]::NewGuid().ToString('N').Substring(0,12)
$previewRoot=Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) 'SurvivalGame\PreviewProfiles'
if(Test-Path -LiteralPath (Join-Path $previewRoot "profile-$probe")){
    throw 'Refusing to use an existing profile for the unautomated startup probe.'
}
$probeLog=Join-Path $output 'normal-preview.log'
$arguments=$prefix+"-HomesteadPreviewProfile=$probe -RenderOffscreen -windowed -ForceRes -ResX=1280 -ResY=720 -nosound -abslog=`"$probeLog`""
$process=Start-Process -FilePath $executable -WorkingDirectory $working -ArgumentList $arguments -PassThru
try {
    $ready=$false
    $limit=[DateTimeOffset]::UtcNow.AddSeconds($TimeoutSeconds)
    while([DateTimeOffset]::UtcNow -lt $limit -and -not $process.HasExited) {
        if(Test-Path -LiteralPath $probeLog) {
            $text=Get-Content -LiteralPath $probeLog -Raw
            if($text -match "SAVE_ROUTING version=1 mode=preview profile=$probe .*automation_input=0 smoke_actor=0 visual_actor=0"){
                $ready=$true;break
            }
        }
        Start-Sleep -Milliseconds 500
    }
    if(-not $ready){throw 'Preview-only startup did not prove the real profile route and normal input/no-actor configuration.'}
    Start-Sleep -Seconds 5
    if($process.HasExited){throw 'Human preview exited automatically rather than staying available for play.'}
    $probePid=$process.Id
} finally {
    if(-not $process.HasExited){Stop-Process -Id $process.Id; $process.WaitForExit()}
    $process.Dispose()
}
$copies=Join-Path $output 'FixtureCopies'
$null=New-Item -ItemType Directory -Path $copies
for($i=0;$i -lt $directories.Count;$i++) {
    $destination=Join-Path $copies "route-$i"
    $null=New-Item -ItemType Directory -Path $destination
    Copy-Item -LiteralPath (Get-ChildItem -LiteralPath $directories[$i] -File).FullName -Destination $destination
}
$cleanup=@($directories[1],$directories[2],(Join-Path $previewRoot "profile-$probe\SaveGames"))
foreach($directory in $cleanup) {
    if(-not (Test-Path -LiteralPath $directory)){continue}
    $parent=Split-Path $directory -Parent
    if((Split-Path $parent -Parent) -ne $previewRoot -or (Split-Path $parent -Leaf) -notmatch '^profile-(verification-[ab]-[a-f0-9]{12}|input-check-[a-f0-9]{12})$'){
        throw "Refusing cleanup outside this run's named synthetic preview namespaces: $directory"
    }
    $files=@(Get-ChildItem -LiteralPath $directory -File)
    if(@($files | Where-Object Name -NotMatch '^Homestead_(Manual|Auto_[012]|Recovery)\.sav(\.bak)?$').Count){
        throw "Unexpected files in synthetic preview fixture; preserved for inspection: $directory"
    }
    foreach($file in $files){Remove-Item -LiteralPath $file.FullName}
    Remove-Item -LiteralPath $directory
    Remove-Item -LiteralPath $parent
}
[ordered]@{
    status='passed'; writeChecks=([regex]::Match($write,'SUCCESS (\d+)').Groups[1].Value -as [int])
    relaunchReadChecks=([regex]::Match($read,'SUCCESS (\d+)').Groups[1].Value -as [int])
    unchangedSaveFiles=$before.Count; invalidStartupCases=$invalid.Count
    fixtureDirectories=$directories; realDefaultAccess='Path resolution only; default IO used a separate synthetic root.'
    previewFixtures='Two fresh known-root preview profiles; actual slots/backups reloaded by a second process, copied, then removed.'
    normalInputProbe=@{profile=$probe; pid=$probePid; automationFlags=$false; stayedAlive=$true; stoppedOnlyOwnedPid=$true}
    limits='Physical-source-style in-process input probes plus real unautomated startup, not a human controller comfort claim.'
} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $output 'preview-save-result.json')
Write-Output "Preview runtime isolation passed; reports and synthetic save copies: $output"
