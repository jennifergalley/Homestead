[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$PackageDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [ValidateSet(1280,3840)][int]$Width = 1280,
    [ValidateSet(720,2160)][int]$Height = 720,
    [switch]$Baseline
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$output = [IO.Path]::GetFullPath($OutputDirectory, $root)
if (Test-Path -LiteralPath $output) { throw 'Feedback fixtures require a fresh output directory.' }
if (($Width -eq 1280) -ne ($Height -eq 720)) { throw 'Use matching720p or4K dimensions.' }
$package = & (Join-Path $PSScriptRoot 'Resolve-PackageDirectory.ps1') -PackageDirectory $PackageDirectory
$exe = Join-Path $package 'SurvivalGame\Binaries\Win64\SurvivalGame.exe'
& (Join-Path $PSScriptRoot 'Set-EngineEnvironment.ps1')
$null = New-Item -ItemType Directory -Path (Join-Path $output 'Graphics')
$config = Join-Path $output 'Graphics\GameUserSettings.ini'
Copy-Item -LiteralPath (Join-Path $root 'Config\DefaultGameUserSettings.ini') -Destination $config
$baselineFlag = if ($Baseline) { '-HomesteadFeedbackBaseline' } else { '' }
$arguments = "-HomesteadSmokeTest -HomesteadFeedbackTest $baselineFlag -HomesteadTestOutput=`"$output`" -GameUserSettingsINI=`"$config`" -UserDir=`"$(Join-Path $output 'EngineUser')`" -unattended -RenderOffscreen -windowed -ForceRes -ResX=$Width -ResY=$Height -nosound -nosplash -abslog=`"$(Join-Path $output 'engine.log')`""
$process = Start-Process -FilePath $exe -WorkingDirectory $package -ArgumentList $arguments -PassThru
Write-Host "Owned feedback fixture PID$($process.Id): $output"
try {
    if (-not $process.WaitForExit(300000)) { throw 'Feedback fixture exceeded its five-minute bound.' }
    $result = Get-Content (Join-Path $output 'smoke-result.txt') -Raw
    if ($process.ExitCode -ne 0 -or $result -notmatch '(?m)^SUCCESS ') { throw "Feedback fixture failed: $output" }
    $actual = ([regex]::Match($result, '(?m)^GRAPHICS_CONFIG=([^\r\n]+)')).Groups[1].Value
    if ([IO.Path]::GetFullPath($actual) -ne $config) { throw 'Graphics config destination was not isolated.' }
    $expected = if ($Baseline) { 2 } else { 13 }
    $layouts = @(Get-ChildItem -LiteralPath $output -Filter '*.layout.json')
    if ($layouts.Count -ne $expected) { throw "Expected $expected actual active-toast captures." }
    Add-Type -AssemblyName System.Drawing
    foreach ($file in $layouts) {
        $layout = Get-Content -LiteralPath $file.FullName -Raw | ConvertFrom-Json
        if (-not $layout.source -or -not $layout.fullTextRendered -or -not $layout.insideViewport) {
            throw "Incomplete or out-of-bounds rendered feedback: $($file.Name)"
        }
        if ([bool]$layout.overlap -ne [bool]$Baseline) { throw "Unexpected feedback intersection: $($file.Name)" }
        $image = [Drawing.Image]::FromFile($file.FullName.Replace('.layout.json','.png'))
        try {
            if ($image.Width -ne $Width -or $image.Height -ne $Height) { throw 'Wrong actual screenshot dimensions.' }
        } finally { $image.Dispose() }
    }
    [ordered]@{ status = 'passed'; baseline = [bool]$Baseline; checks = [regex]::Matches($result,'(?m)^PASS ').Count
        width = $Width; height = $Height; captures = $layouts.Count; graphicsConfig = $actual
        executableSha256 = (Get-FileHash $exe).Hash; pid = $process.Id; arguments = $arguments
        limits = 'Offscreen native Canvas feedback proof; no physical scanout or subjective controller-comfort claim.'
    } | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $output 'feedback-result.json')
    Write-Host "Feedback fixture passed: $output"
} finally {
    if (-not $process.HasExited) { Stop-Process -Id $process.Id }
    $process.Dispose()
    if (Test-Path -LiteralPath $config) { (Get-Item -LiteralPath $config).IsReadOnly = $false }
}
