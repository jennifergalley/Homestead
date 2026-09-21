[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$id = [guid]::NewGuid().ToString('N')
$fixture = Join-Path $root "Build\Releases\launcher-test-$id"
$selectionFile = Join-Path $fixture 'selection.json'
$archive = Join-Path $fixture 'candidate space'
$platform = Join-Path $archive 'Windows'
$binary = Join-Path $platform 'SurvivalGame\Binaries\Win64\SurvivalGame.exe'
$receiptFile = Join-Path $archive 'acceptance-receipt.json'
$proof = Join-Path $archive 'Verification\proof-index.json'
$launcher = Join-Path $root 'Scripts\Start-Preview.ps1'
$null = New-Item -ItemType Directory -Path (Split-Path $binary -Parent), (Split-Path $proof -Parent) -Force
Set-Content -LiteralPath $binary -Value 'Synthetic non-executable launcher fixture.'
Set-Content -LiteralPath $proof -Value '[]'
$hash = (Get-FileHash -LiteralPath $binary).Hash
$selection = @{schemaVersion=1; candidateDirectory=[IO.Path]::GetRelativePath($root,$archive); profile='jenny-review'; executableSha256=$hash}
$receipt = @{schemaVersion=2; verificationStatus='passed'; previewSaveRoutingVersion=1; executableSha256=$hash;
    checkpoint=('a' * 40); privateRemoteVerified=$true; proofIndex='Verification\proof-index.json'; proofIndexSha256=(Get-FileHash $proof).Hash}
$checks = 0
function Write-Fixtures {
    $selection | ConvertTo-Json | Set-Content -LiteralPath $selectionFile
    $receipt | ConvertTo-Json | Set-Content -LiteralPath $receiptFile
}
function Assert-True([bool]$Condition,[string]$Message) {
    if(-not $Condition){throw $Message}; $script:checks++
}
function Assert-Rejected([scriptblock]$Action,[string]$Message) {
    $failed=$false
    try { & $Action | Out-Null } catch { $failed=$true }
    Assert-True $failed $Message
}
try {
    Write-Fixtures
    $plan = & $launcher -SelectionFile $selectionFile -ValidateOnly
    $details = & (Join-Path $root 'Scripts\Resolve-PackageDirectory.ps1') -PackageDirectory $archive -Details
    Assert-True ($details.executable -eq $binary -and $details.configuration -ceq 'Development' -and
        $details.packageDirectory -eq $platform) 'Detailed package resolution preserves the Development path/configuration.'
    Assert-True ($plan.Executable -eq $binary -and $plan.WorkingDirectory -eq $platform) 'Archive and spaced path must resolve exactly.'
    Assert-True ($plan.Arguments.Count -eq 2 -and $plan.Arguments[0] -ceq '-HomesteadPreviewProfile=jenny-review' -and
        $plan.Arguments[1] -ceq '-Res=0x0wf') 'Human preview must have the validated profile and native monitor-sized borderless request only.'
    Assert-True (($plan.Arguments -join ' ') -notmatch 'ResX|ResY|ForceRes|ExecCmds|r\.VSync|r\.ScreenPercentage') 'Window default must not hardcode dimensions or alter other graphics preferences.'
    $windowed = & $launcher -SelectionFile $selectionFile -Windowed -ValidateOnly
    Assert-True ($windowed.Arguments.Count -eq 2 -and $windowed.Arguments[1] -ceq '-windowed') 'Explicit windowed opt-out must replace, not compete with, the borderless resolution argument.'
    Assert-True ($windowed.Profile -ceq 'jenny-review' -and $windowed.Executable -eq $binary) 'Windowed opt-out must preserve the validated profile and candidate.'
    Assert-True (($plan.Arguments -join ' ') -notmatch 'Smoke|Visual|Automat|unattended|quit|ExecCmds') 'No automation/input-isolation/exit flags.'
    foreach($profile in @('a','con','second-profile','abcdefghijklmnopqrstuvwxyz123456')) {
        $p=& $launcher -SelectionFile $selectionFile -Profile $profile -ValidateOnly
        Assert-True ($p.Profile -ceq $profile) "Valid explicit profile: $profile"
    }
    foreach($profile in @('', '../escape','..\escape','C:\escape','Upper','has space','-option',"abc`n",'abc"','abcdefghijklmnopqrstuvwxyz1234567')) {
        Assert-Rejected { & $launcher -SelectionFile $selectionFile -Profile $profile -ValidateOnly } "Invalid profile must fail: $profile"
    }
    $selection.candidateDirectory=[IO.Path]::GetRelativePath($root,$platform); Write-Fixtures
    Assert-True ((& $launcher -SelectionFile $selectionFile -ValidateOnly).Executable -eq $binary) 'Exact Windows directory remains supported.'
    foreach($markerDirectory in @($archive,$platform,$fixture)) {
        $marker=Join-Path $markerDirectory 'REJECTED.txt'
        Set-Content -LiteralPath $marker -Value 'Synthetic rejection.'
        Assert-Rejected { & $launcher -SelectionFile $selectionFile -ValidateOnly } "Rejected ancestor/platform must fail: $markerDirectory"
        Remove-Item -LiteralPath $marker
    }
    $selection.candidateDirectory=[IO.Path]::GetRelativePath($root,$archive); Write-Fixtures
    foreach($path in @('Build\Windows','Build\Releases\..\Windows',$archive,'Build\Releases\missing')) {
        $selection.candidateDirectory=$path; Write-Fixtures
        Assert-Rejected { & $launcher -SelectionFile $selectionFile -ValidateOnly } "Unsafe or missing candidate: $path"
    }
    $selection.candidateDirectory=[IO.Path]::GetRelativePath($root,$archive)
    foreach($change in @(@('verificationStatus','unverified'),@('previewSaveRoutingVersion',0),@('schemaVersion',1),
        @('privateRemoteVerified',$false),@('checkpoint','bad'),@('proofIndex','..\escape.json'),
        @('schemaVersion','2'),@('previewSaveRoutingVersion','1'))) {
        $field=$change[0]; $old=$receipt[$field]; $receipt[$field]=$change[1]; Write-Fixtures
        Assert-Rejected { & $launcher -SelectionFile $selectionFile -ValidateOnly } "Unaccepted/unsupported metadata: $field"
        $receipt[$field]=$old
    }
    $selection.executableSha256='0'*64; Write-Fixtures
    Assert-Rejected { & $launcher -SelectionFile $selectionFile -ValidateOnly } 'Selection hash mismatch must fail.'
    $selection.executableSha256=$hash; Write-Fixtures
    Add-Content -LiteralPath $binary -Value 'Changed binary.'
    Assert-Rejected { & $launcher -SelectionFile $selectionFile -ValidateOnly } 'Changed executable must fail.'
    Set-Content -LiteralPath $binary -Value 'Synthetic non-executable launcher fixture.'
    Add-Content -LiteralPath $proof -Value 'Changed proof.'
    Assert-Rejected { & $launcher -SelectionFile $selectionFile -ValidateOnly } 'Changed proof index must fail.'
    Set-Content -LiteralPath $proof -Value '[]'
    Set-Content -LiteralPath $receiptFile -Value '{broken'
    Assert-Rejected { & $launcher -SelectionFile $selectionFile -ValidateOnly } 'Malformed acceptance receipt must fail.'
    Write-Fixtures
    Remove-Item -LiteralPath $receiptFile
    Assert-Rejected { & $launcher -SelectionFile $selectionFile -ValidateOnly } 'Build receipt alone cannot authorize preview.'
    Write-Fixtures
    $shippingBinary = Join-Path (Split-Path $binary -Parent) 'SurvivalGame-Win64-Shipping.exe'
    Copy-Item -LiteralPath $binary -Destination $shippingBinary
    Assert-Rejected { & $launcher -SelectionFile $selectionFile -ValidateOnly } 'Mixed configuration executables must fail.'
    Remove-Item -LiteralPath $binary
    Assert-True ((& $launcher -SelectionFile $selectionFile -ValidateOnly).Executable -eq $shippingBinary) 'Shipping executable must resolve explicitly with the same hash/receipt guards.'
    $shippingPlan = & $launcher -SelectionFile $selectionFile -ValidateOnly
    $details = & (Join-Path $root 'Scripts\Resolve-PackageDirectory.ps1') -PackageDirectory $archive -Details
    Assert-True ($details.executable -eq $shippingBinary -and $details.configuration -ceq 'Shipping') 'Detailed Shipping resolution uses the actual executable.'
    foreach($runner in @('Test-Game.ps1','Playtest-Visual.ps1')) {
        $qaOutput=Join-Path $fixture ("qa-rejected-"+$runner)
        Assert-Rejected { & (Join-Path $root "Scripts\$runner") -Packaged -PackageDirectory $archive -OutputDirectory $qaOutput } 'Shipping route without explicit QA must fail before execution.'
        $null=New-Item -ItemType Directory -Path $qaOutput -Force
        Assert-Rejected { & (Join-Path $root "Scripts\$runner") -Packaged -ShippingQA -PackageDirectory $archive -OutputDirectory $qaOutput } 'Existing QA output must fail before execution.'
    }
    Assert-True ($shippingPlan.Arguments.Count -eq 3 -and $shippingPlan.Arguments[2] -ceq "-UserDir=$(Join-Path $platform 'SurvivalGame')") 'Shipping must retain candidate-local generated config without moving fixed-root preview saves.'
    Move-Item -LiteralPath $shippingBinary -Destination $binary
    $ambiguous=Join-Path $archive 'SurvivalGame\Binaries\Win64'
    $null=New-Item -ItemType Directory -Path $ambiguous -Force
    Set-Content -LiteralPath (Join-Path $ambiguous 'SurvivalGame.exe') -Value 'Ambiguous stale package.'
    Assert-Rejected { & $launcher -SelectionFile $selectionFile -ValidateOnly } 'Ambiguous package roots must fail.'
    $junction=Join-Path $fixture 'linked-candidate'
    $null=New-Item -ItemType Junction -Path $junction -Target $platform
    try {
        $selection.candidateDirectory=[IO.Path]::GetRelativePath($root,$junction); Write-Fixtures
        Assert-Rejected { & $launcher -SelectionFile $selectionFile -ValidateOnly } 'Candidate junction must not redirect selection.'
    } finally { Remove-Item -LiteralPath $junction }
    Write-Output "Preview launcher: $checks checks passed. No executable was launched."
} finally {
    Get-ChildItem -LiteralPath $fixture -File -Recurse | Remove-Item -Force
    Get-ChildItem -LiteralPath $fixture -Directory -Recurse |
        Sort-Object { $_.FullName.Length } -Descending | Remove-Item
    Remove-Item -LiteralPath $fixture
}
