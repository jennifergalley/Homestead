$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$tag='fixture-guards-'+[guid]::NewGuid().ToString('N')
$automation=Join-Path $root "Saved\Automation\$tag"
$visual=Join-Path $root "Saved\VisualPlaytests\$tag"
$null=New-Item -ItemType Directory -Path $automation,$visual
$bytes=[Text.Encoding]::ASCII.GetBytes("HOMESAV1abcdefgh")
$a=Join-Path $automation 'synthetic-envelope.sav'
$v=Join-Path $visual 'synthetic-envelope.sav'
[IO.File]::WriteAllBytes($a,$bytes)
[IO.File]::WriteAllBytes($v,$bytes)
$checks=0
foreach($source in @($a,$v)) {
    $out=Join-Path $automation "copy-$checks"
    & (Join-Path $root 'Scripts\Initialize-TestWorldFixture.ps1') -SourceSave $source -OutputDirectory $out
    if((Get-FileHash $source).Hash -ne (Get-FileHash (Join-Path $out 'SmokeSave\Homestead_Manual.sav')).Hash){throw 'Copied fixture differs'}
    ++$checks
    $rejected=$false
    try { & (Join-Path $root 'Scripts\Initialize-TestWorldFixture.ps1') -SourceSave $source -OutputDirectory $out }
    catch { $rejected=$_.Exception.Message -like 'Fixture requires an empty isolated*' }
    if(-not $rejected){throw 'Occupied save destination was not rejected'}
    ++$checks
}
$rejected=$false
try { & (Join-Path $root 'Scripts\Initialize-TestWorldFixture.ps1') -SourceSave (Join-Path $root 'README.md') -OutputDirectory (Join-Path $automation 'outside') }
catch { $rejected=$_.Exception.Message -like 'The disclosed fixture must be an existing dedicated save*' }
if(-not $rejected){throw 'Non-test source was not rejected before reading'}
++ $checks
$bad=Join-Path $visual 'invalid-envelope.sav'
[IO.File]::WriteAllBytes($bad,[byte[]](0..15))
$rejected=$false
try { & (Join-Path $root 'Scripts\Initialize-TestWorldFixture.ps1') -SourceSave $bad -OutputDirectory (Join-Path $automation 'bad') }
catch { $rejected=$_.Exception.Message -like 'Fixture does not have the existing Homestead save envelope*' }
if(-not $rejected){throw 'Malformed envelope was not rejected'}
++ $checks
@{passed=$checks;automationFixture=$automation;visualFixture=$visual;limits='Copy/path/envelope guards only; actual CRC/schema validation is native.'}|
    ConvertTo-Json|Set-Content (Join-Path $automation 'result.json')
Write-Output "Test-world fixture: $checks checks passed; synthetic proof=$automation"
