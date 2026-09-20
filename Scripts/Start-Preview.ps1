[CmdletBinding()]
param([string]$SelectionFile = 'Preview.json', [AllowEmptyString()][string]$Profile, [switch]$ValidateOnly)
$ErrorActionPreference = 'Stop'
$parameters = @{ SelectionFile = $SelectionFile }
if ($PSBoundParameters.ContainsKey('Profile')) { $parameters.Profile = $Profile }
$plan = & (Join-Path $PSScriptRoot 'Resolve-Preview.ps1') @parameters
if ($ValidateOnly) { return $plan }
Write-Host "Preview: $($plan.Candidate) | profile: $($plan.Profile) | checkpoint: $($plan.Checkpoint)"
Write-Host 'Persistent isolated preview saves. Original Play.cmd and personal saves are unchanged.'
Write-Host 'Normal controller/keyboard/mouse input; no automation route or automatic quit.'
$start = [Diagnostics.ProcessStartInfo]::new()
$start.FileName = $plan.Executable
$start.WorkingDirectory = $plan.WorkingDirectory
$start.UseShellExecute = $false
foreach ($argument in $plan.Arguments) { $start.ArgumentList.Add($argument) }
$process = [Diagnostics.Process]::Start($start)
try {
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) { throw "Preview exited with code $($process.ExitCode)." }
} finally { $process.Dispose() }
