[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$world = Get-Content -LiteralPath (Join-Path $root 'Source\SurvivalGame\HomesteadWorld.cpp') -Raw
$generation = Get-Content -LiteralPath (Join-Path $root 'Source\SurvivalGame\Simulation\HomesteadWorldGeneration.cpp') -Raw

foreach ($required in @('CreekGroundBlendWeight(World, PX, PY)','CreekWaterHalfWidthCm(World, PY, false)',
    'CreekWaterHalfWidthCm(World, PY, true)','natural-creek-v1','bCreekBank',
    'for (int32 Attempt = 0; Attempt < 1200; ++Attempt)',
    'IsDecorationReserved(CoverState, X, Y, 20, 0, true)',
    'IsDecorationReserved(CoverState, X, Y, 75, 0, true)',
    'SetCollisionEnabled(ECollisionEnabled::NoCollision)')) {
    if ($world -notmatch [regex]::Escape($required)) { throw "Missing natural creek contract: $required" }
}
foreach ($removed in @('Ribbon(2,','Ribbon(3,','FLinearColor(0.27f, 0.235f, 0.14f)')) {
    if ($world -match [regex]::Escape($removed)) { throw "Legacy sandy bank contract remains: $removed" }
}
foreach ($required in @('CreekWaterMinimumHalfWidthCm','CreekWaterMaximumHalfWidthCm',
    'Noise(world, 0, y, 700','Noise(world, 0, y, 900')) {
    if ($generation -notmatch [regex]::Escape($required)) { throw "Missing deterministic creek function: $required" }
}
Write-Output 'PASS natural creek source policy: terrain blend, bounded seam-safe water, no tan bank ribbons, batched noncolliding cover.'
