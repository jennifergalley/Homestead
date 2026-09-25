[CmdletBinding()]
param([switch]$ValidateOnly, [switch]$Windowed, [switch]$Vitruvian,
    [switch]$CMUWalk, [switch]$CMUSlowBlend, [switch]$CMUHeadLevel)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$root = Split-Path $PSScriptRoot -Parent
$archive = Join-Path $root $(if ($CMUHeadLevel) {
    'Build\Releases\20260924-character-first-13'
} elseif ($CMUSlowBlend) {
    'Build\Releases\20260924-character-first-12'
} elseif ($CMUWalk) {
    'Build\Releases\20260924-character-first-11'
} elseif ($Vitruvian) {
    'Build\Releases\20260924-character-first-10'
} else {
    'Build\Releases\20260924-character-first-08'
})
$receiptFile = Join-Path $archive 'build-receipt.json'
if (-not (Test-Path -LiteralPath $receiptFile -PathType Leaf)) {
    throw 'The character-first trial has not been staged. The selected Preview.cmd remains available.'
}
$receipt = Get-Content -LiteralPath $receiptFile -Raw | ConvertFrom-Json
if ($receipt.configuration -cne 'Shipping' -or
    $receipt.status -cne 'Genuine fresh loose-file Cook and Shipping stage; gameplay acceptance and preview promotion are separate.') {
    throw 'The character trial lacks its genuine unselected Shipping stage receipt.'
}
$package = & (Join-Path $PSScriptRoot 'Resolve-PackageDirectory.ps1') -PackageDirectory $archive -Details
if ($package.configuration -cne 'Shipping' -or
    $package.packageDirectory -cne $receipt.packageDirectory -or
    (Get-FileHash -LiteralPath $package.executable -Algorithm SHA256).Hash -cne $receipt.nativeExecutableSha256) {
    throw 'The playable trial no longer matches its Shipping executable receipt.'
}
$profile = [IO.Path]::GetFullPath($(if ($CMUHeadLevel) {
    'Saved\PlaytestProfiles\cmu-head-level-original'
} elseif ($CMUSlowBlend) {
    'Saved\PlaytestProfiles\cmu-slow-blend04'
} elseif ($CMUWalk) {
    'Saved\PlaytestProfiles\cmu-walk03'
} elseif ($Vitruvian) {
    'Saved\PlaytestProfiles\vitruvian-25'
} else {
    'Saved\PlaytestProfiles\character-first-08'
}), $root)
$user = Join-Path $profile 'EngineUser'
$graphics = Join-Path $profile 'Graphics\GameUserSettings.ini'
function Reject-Links([string]$Target) {
    for ($path = $Target; $path -and $path.Length -ge $root.Length; $path = Split-Path $path -Parent) {
        if (Test-Path -LiteralPath $path) {
            $item = Get-Item -LiteralPath $path -Force
            if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw "Trial assets or saves may not pass through a reparse point: $path"
            }
        }
        if ($path -eq $root) { break }
    }
}
foreach ($path in @($receiptFile, $package.executable, $user, $graphics)) { Reject-Links $path }
if ((Test-Path -LiteralPath $graphics) -and
    -not (Test-Path -LiteralPath $graphics -PathType Leaf)) {
    throw 'Isolated trial graphics settings must be an ordinary file.'
}
$arguments = @("-UserDir=$user", "-GameUserSettingsINI=$graphics",
    $(if ($Windowed) { '-windowed' } else { '-Res=0x0wf' }))
if ($Vitruvian) {
    if (@($receipt.cookedPackages | Where-Object {
        $_.path -ceq 'Content\Trials\HeroineVitruvian_20260924_25\SK_TrialVitruvian01_Preferred_Base_Bob.uasset'
    }).Count -ne 1) {
        throw 'The trial receipt does not contain the new face; use the original character launcher instead.'
    }
    $arguments += '-HomesteadHeroineTrialVitruvian01'
}
if ($CMUWalk -and $CMUSlowBlend) { throw 'Select one motion candidate.' }
if ($CMUHeadLevel -and ($CMUWalk -or $CMUSlowBlend -or $Vitruvian)) {
    throw 'The upright-head motion candidate uses the original heroine, not the rejected face.'
}
if ($CMUWalk -or $CMUSlowBlend) {
    if (-not $Vitruvian -or @($receipt.cookedPackages | Where-Object {
        $_.path -ceq 'Content\Trials\HeroineCMUWalk_20260924_03\Animations\AN_Heroine_CMUNormalWalk01.uasset'
    }).Count -ne 1) {
        throw 'The human-motion trial requires both the licensed face and cooked captured walk.'
    }
    if ($CMUSlowBlend -and (-not $receipt.motionGaitReview -or @($receipt.cookedPackages | Where-Object {
        $_.path -ceq 'Content\Trials\HeroineCMUWalk_20260924_03\Animations\AN_Heroine_CMUSlowWalk01.uasset'
    }).Count -ne 1)) {
        throw 'The slow/full motion candidate requires the cooked slow clip and review-capable build.'
    }
    $arguments += '-HomesteadTrialCMUWalk01'
}
if ($CMUHeadLevel) {
    if (-not $receipt.levelHeadMotion -or @($receipt.cookedPackages | Where-Object {
        $_.path -like 'Content\Trials\HeroineCMUWalk_20260924_04\Animations\*.uasset'
    }).Count -ne 2) {
        throw 'The upright-head candidate needs both separate licensed motion clips.'
    }
    $arguments += '-HomesteadTrialCMUWalk01', '-HomesteadTrialCMULevelHead'
}
$plan = [pscustomobject]@{
    Executable = $package.executable
    ExecutableSha256 = $receipt.nativeExecutableSha256
    Arguments = $arguments
    PlaytestData = $profile
    SelectedPreviewUnchanged = $true
}
if ($ValidateOnly) { return $plan }

$null = New-Item -ItemType Directory -Path $user, (Split-Path $graphics -Parent) -Force
if (-not (Test-Path -LiteralPath $graphics)) {
    Copy-Item -LiteralPath (Join-Path $root 'Config\DefaultGameUserSettings.ini') -Destination $graphics
}
Write-Host $(if ($CMUHeadLevel) {
    'Original heroine with experimental upright-head slow/full CMU walk, Sprint, Knife and improved menus.'
} elseif ($CMUSlowBlend) {
    'Separate experimental face and slow/full captured walking: Preferred/Bob, Sprint, Knife and Craft.'
} elseif ($CMUWalk) {
    'Separate experimental face and CMU walking playtest: Preferred/Bob, captured walk, Sprint, Knife and Craft.'
} elseif ($Vitruvian) {
    'Character-first experimental face playtest: Preferred body, straight brown Bob, Sprint, Knife, Craft and living idle.'
} else {
    'Character-first experimental playtest: Sprint, Knife, Craft and living idle.'
})
Write-Host $(if ($CMUHeadLevel) {
    'This uses the original face: the Vitruvian face was rejected. The revised walk still needs your review.'
} else {
    'The replacement face in this older trial was rejected. Preview.cmd and personal saves are untouched.'
})
Write-Host "This trial's saved worlds and settings are isolated under $profile."
$start = [Diagnostics.ProcessStartInfo]::new()
$start.FileName = $package.executable
$start.WorkingDirectory = $package.packageDirectory
$start.UseShellExecute = $false
foreach ($argument in $arguments) { $start.ArgumentList.Add($argument) }
$process = [Diagnostics.Process]::Start($start)
try {
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) { throw "Character trial exited with code $($process.ExitCode)." }
} finally { $process.Dispose() }
