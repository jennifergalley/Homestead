[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$defaults = Join-Path $root 'Config\DefaultGameUserSettings.ini'
$hash = (Get-FileHash $defaults).Hash
$file = Join-Path ([IO.Path]::GetTempPath()) ("homestead-graphics-" + [guid]::NewGuid().ToString('N') + '.ini')
$compare = Join-Path $root 'Scripts\Compare-GraphicsDefaults.ps1'
$checks = 0
try {
    Copy-Item -LiteralPath $defaults -Destination $file
    $result = & $compare -DefaultsFile $defaults -UserFile $file -InitialDefaultsSha256 $hash
    if (-not $result.bytesIdentical -or $result.comparisons.Count -ne 10) { throw 'Exact fixture comparison failed.' }; $checks++
    Set-Content -LiteralPath $file -Value ";METADATA=(Diff=true, UseCommands=true)`n[/Script/Engine.GameUserSettings]`nPreferredFullscreenMode=1"
    $result = & $compare -DefaultsFile $defaults -UserFile $file -InitialDefaultsSha256 $hash
    if ($result.bytesIdentical -or @($result.comparisons | Where-Object { -not $_.inherited }).Count) { throw 'Inherited defaults failed.' }; $checks++
    foreach ($entry in @('bUseVSync=False','FullscreenMode=2','FrameRateLimit=45.000000','ResolutionSizeX=1600','+bUseVSync=True')) {
        Set-Content -LiteralPath $file -Value "[/Script/Engine.GameUserSettings]`n$entry"
        $failed=$false; try { & $compare -DefaultsFile $defaults -UserFile $file -InitialDefaultsSha256 $hash | Out-Null } catch { $failed=$true }
        if (-not $failed) { throw "Invalid preferences accepted: $entry" }; $checks++
    }
    $failed=$false; try { & $compare -DefaultsFile $defaults -UserFile $file -InitialDefaultsSha256 ('0'*64) | Out-Null } catch { $failed=$true }
    if (-not $failed) { throw 'Changed defaults accepted.' }; $checks++
    Write-Output "Graphics defaults: $checks checks passed; no game launched."
} finally { if (Test-Path -LiteralPath $file) { Remove-Item -LiteralPath $file } }
