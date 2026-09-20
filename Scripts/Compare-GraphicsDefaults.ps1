[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$DefaultsFile,
    [Parameter(Mandatory)][string]$UserFile,
    [Parameter(Mandatory)][string]$InitialDefaultsSha256
)
$ErrorActionPreference = 'Stop'
if ((Get-FileHash -LiteralPath $DefaultsFile).Hash -ne $InitialDefaultsSha256) {
    throw 'The inherited defaults differ from the exact initial graphics fixture.'
}
function Read-SimpleIni([string]$Path) {
    $values = @{}; $section = ''
    foreach ($line in Get-Content -LiteralPath $Path) {
        $text = $line.Trim()
        if (-not $text -or $text.StartsWith(';')) { continue }
        if ($text -match '^\[([^\]]+)\]$') { $section = $Matches[1]; continue }
        if (-not $section -or $text -notmatch '^([^=]+)=(.*)$') { throw "Unsupported fixture INI syntax: $Path" }
        $key = $Matches[1].Trim(); $value = $Matches[2].Trim()
        if ($key -match '^[!+\-.]') { throw 'Array/removal commands are not valid in this simple graphics fixture.' }
        $qualified = "$section/$key"
        if ($values.ContainsKey($qualified)) { throw "Duplicate fixture key: $qualified" }
        $values[$qualified] = $value
    }
    return $values
}
$defaults = Read-SimpleIni $DefaultsFile
$overrides = Read-SimpleIni $UserFile
$comparisons = @(
    foreach ($key in ($defaults.Keys | Sort-Object)) {
        $inherited = -not $overrides.ContainsKey($key)
        $effective = if ($inherited) { $defaults[$key] } else { $overrides[$key] }
        if ($effective -ine $defaults[$key]) { throw "Changed requested graphics preference: $key ($($defaults[$key]) -> $effective)" }
        [ordered]@{key=$key; requested=$defaults[$key]; effective=$effective; inherited=$inherited}
    }
)
if (-not $comparisons.Count) { throw 'No requested graphics preferences were checked.' }
[ordered]@{
    status='passed'; defaultsSha256=$InitialDefaultsSha256; generatedSha256=(Get-FileHash -LiteralPath $UserFile).Hash
    bytesIdentical=((Get-FileHash -LiteralPath $UserFile).Hash -eq $InitialDefaultsSha256)
    comparisons=$comparisons
    scope='Only exact requested keys in the hash-verified initial defaults, with Unreal inherited-default semantics. Additional engine-generated fields are not claimed byte-identical.'
}
