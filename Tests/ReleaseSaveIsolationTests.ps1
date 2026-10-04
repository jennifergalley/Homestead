[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$fixture = Join-Path 'E:\CopilotScratch\e528fd4a-5aed-4c95-9463-a37941afc00b' ('release-save-isolation-' + [guid]::NewGuid().ToString('N'))
$guard = Join-Path $root 'Scripts\Assert-ReleaseSaveIsolation.ps1'
function Make-Package([string]$Name) {
    $platform = Join-Path $fixture "$Name\Windows"
    $binary = Join-Path $platform 'SurvivalGame\Binaries\Win64\SurvivalGame-Win64-Shipping.exe'
    $saveRoot = Join-Path $platform 'SurvivalGame\Saved\SaveGames'
    $null = New-Item -ItemType Directory -Force -Path (Split-Path $binary), $saveRoot
    Set-Content -LiteralPath $binary -Value $Name
    return $platform
}
function Assert-Rejected([scriptblock]$Action) {
    try { & $Action | Out-Null } catch { return }
    throw 'Unsafe release routing was accepted.'
}
try {
    $candidate = Make-Package 'candidate'
    $rollback = Make-Package 'rollback'
    $commaCandidate = Make-Package 'release,candidate'
    $commaRollback = Make-Package 'release,rollback'
    $ok = & $guard -CandidatePackage $candidate -RollbackPackage $rollback `
        -CandidateArguments "-UserDir=`"$(Join-Path $candidate 'SurvivalGame')`"" `
        -RollbackArguments "-UserDir=`"$(Join-Path $rollback 'SurvivalGame')`""
    if ($ok.Candidate.SaveRoot -eq $ok.Rollback.SaveRoot) { throw 'Distinct roots were not preserved.' }
    Assert-Rejected { & $guard -CandidatePackage $candidate -RollbackPackage $rollback `
        -CandidateArguments "-UserDir=`"$(Join-Path $candidate 'SurvivalGame')`" -HomesteadPreviewProfile=shared" `
        -RollbackArguments "-UserDir=`"$(Join-Path $rollback 'SurvivalGame')`"" }
    Assert-Rejected { & $guard -CandidatePackage $candidate -RollbackPackage $rollback `
        -CandidateArguments "-UserDir=`"$(Join-Path $candidate 'SurvivalGame')`" `"-HomesteadPreviewProfile=shared`"" `
        -RollbackArguments "-UserDir=`"$(Join-Path $rollback 'SurvivalGame')`"" }
    Assert-Rejected { & $guard -CandidatePackage $candidate -RollbackPackage $rollback `
        -CandidateArguments "-UserDir=`"$(Join-Path $rollback 'SurvivalGame')`"" `
        -RollbackArguments "-UserDir=`"$(Join-Path $rollback 'SurvivalGame')`"" }
    Assert-Rejected { & $guard -CandidatePackage $candidate -RollbackPackage $rollback `
        -CandidateArguments "/UserDir=E:\shared -UserDir=`"$(Join-Path $candidate 'SurvivalGame')`"" `
        -RollbackArguments "-UserDir=`"$(Join-Path $rollback 'SurvivalGame')`"" }
    Assert-Rejected { & $guard -CandidatePackage $candidate -RollbackPackage $rollback `
        -CandidateArguments "UserDir=E:\shared -UserDir=`"$(Join-Path $candidate 'SurvivalGame')`"" `
        -RollbackArguments "-UserDir=`"$(Join-Path $rollback 'SurvivalGame')`"" }
    Assert-Rejected { & $guard -CandidatePackage $candidate -RollbackPackage $rollback `
        -CandidateArguments "--UserDir=E:\shared -UserDir=`"$(Join-Path $candidate 'SurvivalGame')`"" `
        -RollbackArguments "-UserDir=`"$(Join-Path $rollback 'SurvivalGame')`"" }
    Assert-Rejected { & $guard -CandidatePackage $candidate -RollbackPackage $rollback `
        -CandidateArguments "-UserDir= E:\shared -UserDir=`"$(Join-Path $candidate 'SurvivalGame')`"" `
        -RollbackArguments "-UserDir=`"$(Join-Path $rollback 'SurvivalGame')`"" }
    Assert-Rejected { & $guard -CandidatePackage $candidate -RollbackPackage $rollback `
        -CandidateArguments "-Note=`"UserDir=$(Join-Path $candidate 'SurvivalGame') `" " `
        -RollbackArguments "-UserDir=`"$(Join-Path $rollback 'SurvivalGame')`"" }
    Assert-Rejected { & $guard -CandidatePackage $candidate -RollbackPackage $rollback `
        -CandidateArguments "$([char]0x00E9)UserDir=E:\shared -UserDir=`"$(Join-Path $candidate 'SurvivalGame')`"" `
        -RollbackArguments "-UserDir=`"$(Join-Path $rollback 'SurvivalGame')`"" }
    Assert-Rejected { & $guard -CandidatePackage $commaCandidate -RollbackPackage $commaRollback `
        -CandidateArguments "-UserDir=$(Join-Path $commaCandidate 'SurvivalGame')" `
        -RollbackArguments "-UserDir=$(Join-Path $commaRollback 'SurvivalGame')" }
    Assert-Rejected { & $guard -CandidatePackage $candidate -RollbackPackage $rollback `
        -CandidateArguments "-UserDir= `"$(Join-Path $candidate 'SurvivalGame')`"" `
        -RollbackArguments "-UserDir=`"$(Join-Path $rollback 'SurvivalGame')`"" }
    Assert-Rejected { & $guard -CandidatePackage $candidate -RollbackPackage $rollback `
        -CandidateArguments "-UserDir=$([char]0x00A0)$(Join-Path $candidate 'SurvivalGame')" `
        -RollbackArguments "-UserDir=`"$(Join-Path $rollback 'SurvivalGame')`"" }
    Assert-Rejected { & $guard -CandidatePackage $candidate -RollbackPackage ("\\?\" + $candidate) `
        -CandidateArguments "-UserDir=`"$(Join-Path $candidate 'SurvivalGame')`"" `
        -RollbackArguments "-UserDir=`"$(Join-Path $candidate 'SurvivalGame')`"" }
    $forwardAlias = ('//?/' + $candidate.Replace('\', '/'))
    Assert-Rejected { & $guard -CandidatePackage $candidate -RollbackPackage $forwardAlias `
        -CandidateArguments "-UserDir=`"$(Join-Path $candidate 'SurvivalGame')`"" `
        -RollbackArguments "-UserDir=`"$(Join-Path $candidate 'SurvivalGame')`"" }
    Write-Output 'Release save isolation: 15 checks passed.'
} finally {
    if (Test-Path -LiteralPath $fixture) { Remove-Item -LiteralPath $fixture -Recurse -Force }
}
