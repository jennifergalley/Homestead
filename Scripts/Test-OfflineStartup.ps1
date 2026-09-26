[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$PackageDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [Parameter(Mandatory)][string]$RunId,
    [Parameter(Mandatory)][string]$FixtureSave,
    [Parameter(Mandatory)][ValidatePattern('^[A-Fa-f0-9]{64}$')][string]$FixtureSha256,
    [switch]$VitruvianTrial,
    [switch]$CMUWalk
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent
$run = & (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
if (-not $run.allowWork -or $run.id -ne $RunId -or
    ($run.completionPolicy -ne 'until-complete' -and
        [DateTimeOffset]::UtcNow.AddMinutes(3) -gt [DateTimeOffset]$run.deadlineUtc)) {
    throw 'Run permission/deadline does not allow the bounded startup probe.'
}
$output = [IO.Path]::GetFullPath($OutputDirectory, $root)
if (Test-Path -LiteralPath $output) { throw 'Use a fresh startup probe output.' }
$package = & (Join-Path $PSScriptRoot 'Resolve-PackageDirectory.ps1') -PackageDirectory $PackageDirectory
$exe = Join-Path $package 'SurvivalGame\Binaries\Win64\SurvivalGame-Win64-Shipping.exe'
if (-not (Test-Path -LiteralPath $exe)) { throw 'The offline probe requires the actual Shipping executable.' }
$trialMesh = Join-Path $package 'SurvivalGame\Content\Trials\HeroineVitruvian_20260924_25\SK_TrialVitruvian01_Preferred_Base_Bob.uasset'
if ($VitruvianTrial -and -not (Test-Path -LiteralPath $trialMesh -PathType Leaf)) {
    throw 'The normal-startup face trial requires its cooked and staged skeletal mesh.'
}
if ($CMUWalk) {
    $motionMesh = Join-Path $package 'SurvivalGame\Content\Trials\HeroineCMUWalk_20260924_03\Animations\AN_Heroine_CMUNormalWalk01.uasset'
    if (-not $VitruvianTrial -or -not (Test-Path -LiteralPath $motionMesh -PathType Leaf)) {
        throw 'The normal-startup walk trial requires its packaged licensed face and motion.'
    }
}
$profileHash = [Convert]::ToHexString([Security.Cryptography.MD5]::HashData([Text.Encoding]::UTF8.GetBytes($output.Replace('\','/'))))
$profile = 'offline-' + $profileHash.Substring(0,12).ToLowerInvariant()
$saveDirectory = Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) "SurvivalGame\PreviewProfiles\profile-$profile\SaveGames"
if (Test-Path -LiteralPath (Split-Path $saveDirectory -Parent)) { throw 'Synthetic profile already exists; refusing to overwrite it.' }
$fixture = [IO.Path]::GetFullPath($FixtureSave, $root)
$testRoot = [IO.Path]::GetFullPath((Join-Path $root 'Saved\Automation')) + '\'
if (-not $fixture.StartsWith($testRoot, [StringComparison]::OrdinalIgnoreCase) -or
    -not (Test-Path -LiteralPath $fixture -PathType Leaf)) {
    throw 'Startup fixture must be an existing isolated automation save.'
}
for ($item = Get-Item -LiteralPath $fixture; $item -and $item.FullName.Length -ge $root.Length;
    $item = if ($item -is [IO.DirectoryInfo]) { $item.Parent } else { $item.Directory }) {
    if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
        throw 'Startup fixture/ancestor must be an ordinary local file.'
    }
}
$fixtureHash = $FixtureSha256.ToUpperInvariant()
if ((Get-FileHash -LiteralPath $fixture).Hash -ne $fixtureHash) { throw 'Prepared test-world provenance differs.' }
$null = New-Item -ItemType Directory -Path (Join-Path $output 'Graphics'), $saveDirectory
$config = Join-Path $output 'Graphics\GameUserSettings.ini'
Copy-Item -LiteralPath (Join-Path $root 'Config\DefaultGameUserSettings.ini') -Destination $config
Copy-Item -LiteralPath $fixture -Destination (Join-Path $saveDirectory 'Homestead_Manual.sav')
$configHash = (Get-FileHash -LiteralPath $config).Hash
& (Join-Path $PSScriptRoot 'Set-EngineEnvironment.ps1')
# Warm the read-only providers before startup; inspect only the owned probe PID afterward.
$null = Get-CimInstance -Namespace 'root\StandardCimv2' -ClassName MSFT_NetTCPConnection -Filter "OwningProcess=$PID"
$null = Get-CimInstance -Namespace 'root\StandardCimv2' -ClassName MSFT_NetUDPEndpoint -Filter "OwningProcess=$PID"
$arguments = "-HomesteadPreviewProfile=$profile -HomesteadStartupProbe=`"$output`" -GameUserSettingsINI=`"$config`" -UserDir=`"$(Join-Path $output 'EngineUser')`" -RenderOffscreen -windowed -ForceRes -ResX=1280 -ResY=720 -nosound -nosplash -unattended"
if ($VitruvianTrial) { $arguments += ' -HomesteadHeroineTrialVitruvian01' }
if ($CMUWalk) { $arguments += ' -HomesteadTrialCMUWalk01' }
$samples = [Collections.Generic.List[object]]::new()
$started = [DateTimeOffset]::UtcNow
$process = Start-Process -FilePath $exe -WorkingDirectory $package -ArgumentList $arguments -PassThru
[ordered]@{pid=$process.Id; startedUtc=$started.ToString('o'); arguments=$arguments; profile=$profile; saveDirectory=$saveDirectory
    fixture=$fixture; fixtureSha256=$fixtureHash; executable=$exe; executableSha256=(Get-FileHash $exe).Hash
    graphicsFile=$config; graphicsBeforeSha256=$configHash; normalHumanArgumentsNotUsed='-Res=0x0wf would call ShowWindow; visible borderless confirmation is human-pending.'
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $output 'launch.json')
Write-Host "Owned Shipping startup probe PID $($process.Id): $output"
try {
    $limit = $started.AddSeconds(90)
    while (-not $process.HasExited) {
        $tcp = @(Get-CimInstance -Namespace 'root\StandardCimv2' -ClassName MSFT_NetTCPConnection -Filter "OwningProcess=$($process.Id)" |
            Select-Object LocalAddress,LocalPort,RemoteAddress,RemotePort,State)
        $udp = @(Get-CimInstance -Namespace 'root\StandardCimv2' -ClassName MSFT_NetUDPEndpoint -Filter "OwningProcess=$($process.Id)" |
            Select-Object LocalAddress,LocalPort)
        $samples.Add([ordered]@{utc=[DateTimeOffset]::UtcNow.ToString('o'); tcp=$tcp; udp=$udp})
        if ($tcp.Count -or $udp.Count) { throw 'Unexpected owned network endpoint. Stop and report before any further launch.' }
        $state = Get-Content -LiteralPath (Join-Path $root 'Automation\run.json') -Raw | ConvertFrom-Json
        if ($state.state -ne 'running' -or
            ($state.completionPolicy -ne 'until-complete' -and
                [DateTimeOffset]::UtcNow -ge [DateTimeOffset]$state.deadlineUtc)) {
            Set-Content -LiteralPath (Join-Path $output 'stop-probe.txt') -Value 'Run control stopped the owned probe.'
        }
        if ([DateTimeOffset]::UtcNow -gt $limit) { throw 'Owned startup probe exceeded its bounded timeout.' }
        Start-Sleep -Milliseconds 150
    }
    $process.WaitForExit()
    $native = Get-Content -LiteralPath (Join-Path $output 'startup-probe.json') -Raw | ConvertFrom-Json
    if ($process.ExitCode -ne 0 -or $native.status -ne 'passed' -or -not $native.shipping -or $native.traceCompiled) {
        throw 'Shipping native acceptance/compiled trace guard failed.'
    }
    if ($native.actualWindowMode -ne 2 -or $native.viewportWidth -ne 1280 -or $native.viewportHeight -ne 720 -or
        $native.savedWindowMode -ne 1 -or $native.vsyncPreference -or $native.frameLimit -ne 60 -or $native.'r.VSync' -ne '0' -or
        $native.viewMode -ne 3 -or $native.litGuardTicks -lt 1 -or -not $native.heroinePresent -or
        $native.saveDispatches -ne 1 -or $native.loadDispatches -ne 1 -or
        $native.savedSimulationMd5 -ne $native.loadedSimulationMd5 -or $native.profile -cne $profile) {
        throw 'Shipping display/defaults/Lit/profile/save checks failed.'
    }
    $graphics = & (Join-Path $PSScriptRoot 'Compare-GraphicsDefaults.ps1') -DefaultsFile (Join-Path $root 'Config\DefaultGameUserSettings.ini') -UserFile $config -InitialDefaultsSha256 $configHash
    $graphics | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $output 'graphics-comparison.json')
    $shots = @(Get-ChildItem -LiteralPath (Join-Path $output 'EngineUser') -Filter '*.png' -File -Recurse -ErrorAction SilentlyContinue)
    if ($shots.Count) { throw 'F9 unexpectedly produced screenshot files.' }
    $copy = Join-Path $output 'ProfileSaveCopies'
    $null = New-Item -ItemType Directory -Path $copy
    Copy-Item -LiteralPath (Get-ChildItem -LiteralPath $saveDirectory -File).FullName -Destination $copy
    [ordered]@{status='passed'; samples=$samples.Count; elapsedSeconds=([DateTimeOffset]::UtcNow-$started).TotalSeconds
        native=$native; requestedGraphicsValuesUnchanged=$true; graphicsBytesIdentical=$graphics.bytesIdentical; observedTcpEndpoints=0; observedUdpEndpoints=0
        limits='Bounded owned-PID endpoint sampling plus compiled trace-disabled Shipping proof; not packet capture or proof no brief connection can ever occur. Offscreen windowed only; actual visible borderless and human control comfort are not claimed.'
    } | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath (Join-Path $output 'offline-result.json')
} finally {
    $samples | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $output 'owned-endpoints.json')
    if (-not $process.HasExited) { Stop-Process -Id $process.Id; $process.WaitForExit() }
    $process.Dispose()
}
