[CmdletBinding()]
param(
    [string]$EngineRoot,
    [switch]$Packaged,
    [string]$PackageDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [ValidateRange(30,600)][int]$TimeoutSeconds = 240
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$output = [IO.Path]::GetFullPath($OutputDirectory, $root)
if (Test-Path -LiteralPath $output) { throw 'Use a fresh camera-preference fixture directory.' }
$null = New-Item -ItemType Directory -Path (Join-Path $output 'Graphics') -Force
$config = Join-Path $output 'Graphics\GameUserSettings.ini'
@'
[/Script/Engine.GameUserSettings]
bUseVSync=False
bUseDynamicResolution=False
ResolutionSizeX=1600
ResolutionSizeY=900
FullscreenMode=2
FrameRateLimit=57.000000
Version=5

[ScalabilityGroups]
sg.ResolutionQuality=73
sg.ViewDistanceQuality=2
'@ | Set-Content -LiteralPath $config
Copy-Item -LiteralPath $config -Destination (Join-Path $output 'initial-fixture.ini')

if ($Packaged) {
    $package = & (Join-Path $PSScriptRoot 'Resolve-PackageDirectory.ps1') -PackageDirectory $PackageDirectory -Details
    $working = $package.packageDirectory
    $exe = $package.executable
    $prefix = ''
} else {
    $engine = & (Join-Path $PSScriptRoot 'Resolve-Engine.ps1') -EngineRoot $EngineRoot
    $working = $root
    $exe = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor.exe'
    $offline = (& (Join-Path $PSScriptRoot 'Get-UnrealOfflineArguments.ps1')) -join ' '
    $prefix = "`"$(Join-Path $root 'SurvivalGame.uproject')`" /Game/SurvivalGame/Maps/Homestead -game $offline "
}
& (Join-Path $PSScriptRoot 'Set-EngineEnvironment.ps1')

function Invoke-CameraPhase([string]$Name, [string]$Root, [string]$Ini) {
    $null = New-Item -ItemType Directory -Path $Root -Force
    $log = Join-Path $Root "$Name.log"
    $arguments = $prefix + "-HomesteadSmokeTest -HomesteadCameraPreferenceTest -HomesteadCameraPreferencePhase=$Name " +
        "-HomesteadTestOutput=`"$Root`" -GameUserSettingsINI=`"$Ini`" -UserDir=`"$(Join-Path $Root 'EngineUser')`" " +
        "-unattended -RenderOffscreen -windowed -ForceRes -ResX=1280 -ResY=720 -nosound -nosplash -abslog=`"$log`""
    if ($Packaged -and $package.configuration -eq 'Shipping') { $arguments += ' -HomesteadShippingQA' }
    $shipping = $Packaged -and $package.configuration -eq 'Shipping'
    $process = if ($shipping) {
        & (Join-Path $PSScriptRoot 'Invoke-ShippingQA.ps1') -PackageDirectory $working `
            -OutputDirectory $Root -Arguments $arguments
    } else {
        Start-Process -FilePath $exe -WorkingDirectory $working -ArgumentList $arguments -PassThru
    }
    try {
        if (-not $shipping -and -not $process.WaitForExit($TimeoutSeconds * 1000)) {
            throw "Camera preference phase timed out: $Name PID$($process.Id)"
        }
        $resultPath = Join-Path $Root 'smoke-result.txt'
        if (-not (Test-Path -LiteralPath $resultPath)) {
            throw "Camera preference phase produced no result: $Name"
        }
        $result = Get-Content -LiteralPath $resultPath -Raw
        if ($process.ExitCode -ne 0 -or $result -notmatch '(?m)^SUCCESS ') {
            throw "Camera preference phase failed: $Name. See $log"
        }
        $actual = ([regex]::Match($result, '(?m)^CAMERA_CONFIG=([^\r\n]+)')).Groups[1].Value.Replace('/','\')
        if ([IO.Path]::GetFullPath($actual) -ne $Ini) {
            throw "Camera preference phase used the wrong settings destination: $Name"
        }
        Move-Item -LiteralPath $resultPath -Destination (Join-Path $Root "$Name-result.txt")
        return [ordered]@{
            phase = $Name
            processId = $process.Id
            checks = [regex]::Matches($result, '(?m)^PASS ').Count
            arguments = $arguments
        }
    } finally {
        if (-not $shipping) {
            if (-not $process.HasExited) { Stop-Process -Id $process.Id }
            $process.Dispose()
        }
        if (Test-Path -LiteralPath $Ini) { (Get-Item -LiteralPath $Ini).IsReadOnly = $false }
    }
}

$runs = @()
$writeRoot = if ($Packaged) { Join-Path $output 'write' } else { $output }
$readRoot = if ($Packaged) { Join-Path $output 'read' } else { $output }
$writeConfig = if ($Packaged) { Join-Path $writeRoot 'Graphics\GameUserSettings.ini' } else { $config }
$readConfig = if ($Packaged) { Join-Path $readRoot 'Graphics\GameUserSettings.ini' } else { $config }
if ($Packaged) {
    $null = New-Item -ItemType Directory -Path (Split-Path $writeConfig -Parent) -Force
    Copy-Item -LiteralPath (Join-Path $output 'initial-fixture.ini') -Destination $writeConfig
}
$runs += Invoke-CameraPhase 'write' $writeRoot $writeConfig
if ($Packaged) {
    $null = New-Item -ItemType Directory -Path (Split-Path $readConfig -Parent) -Force
    Copy-Item -LiteralPath $writeConfig -Destination $readConfig
}
$runs += Invoke-CameraPhase 'read' $readRoot $readConfig
$saved = Get-Content -LiteralPath $readConfig -Raw
$sensitivityMatch = [regex]::Match($saved, '(?ms)\[Homestead\.Camera\].*?^Sensitivity=([0-9.]+)\s*$')
$savedSensitivity = $sensitivityMatch.Success -and
    [Math]::Abs([double]::Parse($sensitivityMatch.Groups[1].Value,
        [Globalization.CultureInfo]::InvariantCulture) - 1.2) -lt 0.001
$savedInversion = $saved -match '(?ms)\[Homestead\.Camera\].*?^InvertY=True\s*$'
if (-not ($savedSensitivity -and $savedInversion)) {
    throw 'Final camera preferences were not preserved for the second process.'
}
foreach ($sentinel in @('ResolutionSizeX=1600','ResolutionSizeY=900',
    'FrameRateLimit=57.000000','sg.ResolutionQuality=73','sg.ViewDistanceQuality=2')) {
    if ($saved -notmatch "(?m)^$([regex]::Escape($sentinel))\s*$") {
        throw "Unrelated synthetic preference changed or disappeared: $sentinel"
    }
}

$invalidRoot = Join-Path $output 'invalid'
$null = New-Item -ItemType Directory -Path (Join-Path $invalidRoot 'Graphics') -Force
$invalidConfig = Join-Path $invalidRoot 'Graphics\GameUserSettings.ini'
Copy-Item -LiteralPath (Join-Path $output 'initial-fixture.ini') -Destination $invalidConfig
Add-Content -LiteralPath $invalidConfig -Value "`n[Homestead.Camera]`nSensitivity=NaN`nInvertY=Maybe"
$runs += Invoke-CameraPhase 'invalid' $invalidRoot $invalidConfig
$invalidAfter = Get-Content -LiteralPath $invalidConfig -Raw
$invalidSensitivity = $invalidAfter -match '(?ms)\[Homestead\.Camera\].*?^Sensitivity=NaN\s*$'
$invalidInversion = $invalidAfter -match '(?ms)\[Homestead\.Camera\].*?^InvertY=Maybe\s*$'
if (-not ($invalidSensitivity -and $invalidInversion)) {
    throw 'Invalid camera properties were rewritten during startup fallback.'
}

[ordered]@{
    status = 'passed'
    executableSha256 = (Get-FileHash -LiteralPath $exe).Hash
    runs = $runs
    finalSensitivity = 1.2
    finalInvertY = $true
    settingsFile = $readConfig
    isolation = 'Explicit synthetic GameUserSettingsINI, UserDir and smoke save routes only.'
    limits = 'Mapped offscreen input and real native Slate controls; physical mouse feel remains ordinary-play acceptance.'
} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $output 'camera-preference-result.json')
Write-Output "Camera preference fixtures passed: $output"
