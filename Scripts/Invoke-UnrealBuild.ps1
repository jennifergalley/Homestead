<#
.SYNOPSIS
Builds one or more Unreal targets for this worktree through Build.bat, in a single UnrealBuildTool run.
.DESCRIPTION
All worktrees on this PC share one UnrealBuildTool mutex (it is keyed on the engine's UBT install, not on
the project), so every Build.bat call queues behind every other worktree's build. This wrapper keeps that
queue as short as possible:

- It skips the build entirely when the targets were already built by this script from exactly the
  current sources. The stamp is the git tree hash of Source\ and SurvivalGame.uproject, including
  uncommitted and untracked files, plus the engine version and the target list. A no-op Build.bat
  still waits for the mutex (often 10 minutes or more while other lanes build), so skipping it matters.
  Pass -Force to build anyway.
- Several targets (for example the editor and the game) build in one UBT run: one queue wait, one UBT
  start-up, one action graph.
- It passes -UBADisableRemote. Without it every local build also opens a UBA listener on 0.0.0.0:1345.
- It keeps this worktree's UBT log in Saved\Logs\UnrealBuildTool-<targets>.log. The default log,
  %LOCALAPPDATA%\UnrealBuildTool\Log.txt, is shared by every worktree and overwritten by the next build.
- While it waits, it names the build it is queued behind.
#>
[CmdletBinding()]
param(
    [string[]]$Target = @('SurvivalGameEditor'),
    [ValidateSet('Development', 'Shipping', 'DebugGame')][string]$Configuration = 'Development',
    [string]$EngineRoot,
    [switch]$Force,
    # Report whether the targets are current (True/False) without building.
    [switch]$CheckOnly,
    [string[]]$ExtraArguments = @()
)
$ErrorActionPreference = 'Stop'
# pwsh -File passes 'A,B' as one string.
$Target = @($Target | ForEach-Object { $_ -split '[,+]' } | Where-Object { $_ })
$root = Split-Path $PSScriptRoot -Parent
$engine = & (Join-Path $PSScriptRoot 'Resolve-Engine.ps1') -EngineRoot $EngineRoot
$project = Join-Path $root 'SurvivalGame.uproject'
$name = ($Target -join '+') + "-$Configuration"
# One line per target: <target> <source stamp> <receipt write time>.
$stampFile = Join-Path $root "Binaries\Win64\Homestead-$Configuration.source-stamps"

function Get-SourceStamp {
    # Hash the working copy of Source\ and the .uproject (tracked, modified and untracked files) through
    # a throwaway copy of the index, so the repository's own index is never touched.
    $index = Join-Path ([IO.Path]::GetTempPath()) ("homestead-stamp-" + [guid]::NewGuid().ToString('N'))
    $previous = $env:GIT_INDEX_FILE
    try {
        $real = (& git -C $root rev-parse --path-format=absolute --git-path index).Trim()
        if (Test-Path -LiteralPath $real) { Copy-Item -LiteralPath $real -Destination $index }
        $env:GIT_INDEX_FILE = $index
        & git -C $root add -A -- Source SurvivalGame.uproject 2>$null
        if ($LASTEXITCODE -ne 0) { return $null }
        $full = (& git -C $root write-tree).Trim()
        if ($LASTEXITCODE -ne 0) { return $null }
        $tree = (& git -C $root rev-parse "${full}:Source" "${full}:SurvivalGame.uproject") -join ' '
        if ($LASTEXITCODE -ne 0) { return $null }
    }
    finally {
        if ($null -eq $previous) { Remove-Item Env:GIT_INDEX_FILE -ErrorAction SilentlyContinue } else { $env:GIT_INDEX_FILE = $previous }
        Remove-Item -LiteralPath $index -Force -ErrorAction SilentlyContinue
    }
    $version = Get-Content -Raw -LiteralPath (Join-Path $engine 'Engine\Build\Build.version')
    $text = "$tree`n$Configuration`n$($ExtraArguments -join ' ')`n$version"
    $sha = [Security.Cryptography.SHA256]::Create()
    return [BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($text))).Replace('-', '')
}

$stamp = Get-SourceStamp
$suffix = if ($Configuration -eq 'Development') { '' } else { "-Win64-$Configuration" }
function Get-ReceiptTime([string]$TargetName) {
    $receipt = Join-Path $root "Binaries\Win64\$TargetName$suffix.target"
    if (Test-Path -LiteralPath $receipt) { (Get-Item -LiteralPath $receipt).LastWriteTimeUtc.Ticks } else { 'missing' }
}
$recorded = @{}
if (Test-Path -LiteralPath $stampFile) {
    foreach ($line in Get-Content -LiteralPath $stampFile) {
        $parts = $line -split ' '
        if ($parts.Count -eq 3) { $recorded[$parts[0]] = "$($parts[1]) $($parts[2])" }
    }
}
# The receipt's write time is part of the check: if anything else rebuilt a target since (a direct
# Build.bat, an IDE), its binaries may come from other sources, so build again (usually a quick no-op).
$stale = @($Target | Where-Object { $recorded[$_] -ne "$stamp $(Get-ReceiptTime $_)" -or (Get-ReceiptTime $_) -eq 'missing' })
if ($CheckOnly) { return [bool]($stamp -and -not $stale.Count) }
if (-not $Force -and $stamp -and -not $stale.Count) {
    Write-Host "$($Target -join ', ') ($Configuration) is already built from these sources; skipping UnrealBuildTool (use -Force to build anyway)."
    return
}
$ahead = @(Get-CimInstance Win32_Process -Filter "Name = 'dotnet.exe'" -ErrorAction SilentlyContinue |
    Where-Object { $_.CommandLine -match 'UnrealBuildTool\.dll' -and $_.CommandLine -notmatch '-Mode=' })
if ($ahead.Count) {
    $who = $ahead | ForEach-Object {
        if ($_.CommandLine -match 'copilot-worktrees\\SurvivalGame\\([^\\]+)\\') { $Matches[1] } else { "PID $($_.ProcessId)" }
    }
    Write-Host "UnrealBuildTool is busy ($($who -join ', ')). Every worktree shares one UBT mutex, so this build waits its turn."
}

$logDirectory = Join-Path $root 'Saved\Logs'
$null = New-Item -ItemType Directory -Path $logDirectory -Force
$log = Join-Path $logDirectory "UnrealBuildTool-$name.log"
$common = @('-WaitMutex', '-NoHotReloadFromIDE', '-NoUBA', '-NoXGE', '-NoFASTBuild', '-UBADisableRemote', "-Log=$log") + $ExtraArguments
# UBT builds "A+B" as several targets in one run.
$arguments = @(($Target -join '+'), 'Win64', $Configuration, "-Project=$project") + $common
$build = Join-Path $engine 'Engine\Build\BatchFiles\Build.bat'
$started = Get-Date
& $build @arguments
$code = $LASTEXITCODE
if ($code -ne 0) { throw "UnrealBuildTool failed for $($Target -join ', ') ($code). Log: $log" }
if ($stamp) {
    foreach ($built in $Target) { $recorded[$built] = "$stamp $(Get-ReceiptTime $built)" }
    Set-Content -LiteralPath $stampFile -Value ($recorded.Keys | Sort-Object | ForEach-Object { "$_ $($recorded[$_])" }) -Encoding ascii
}
Write-Host ("Built $($Target -join ', ') in {0:N0} s, including any wait for other worktrees. Log: $log" -f ((Get-Date) - $started).TotalSeconds)
