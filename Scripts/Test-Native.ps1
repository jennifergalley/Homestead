[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$vswhere = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio Build Tools are missing.' }
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'The C++ tools workload is missing from Visual Studio.' }
$cmake = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ctest = Join-Path (Split-Path $cmake -Parent) 'ctest.exe'
if (-not (Test-Path -LiteralPath $cmake)) { throw 'The Visual Studio CMake component is missing.' }
$build = Join-Path $root 'Build\Native'
$cache = Join-Path $build 'CMakeCache.txt'
$configure = @('-S', $root, '-B', $build, '-G', 'Visual Studio 17 2022')
if (Test-Path -LiteralPath $cache) {
    $platform = Get-Content -LiteralPath $cache |
        Where-Object { $_ -match '^CMAKE_GENERATOR_PLATFORM:INTERNAL=' } |
        ForEach-Object { ($_ -split '=', 2)[1] }
    if ($platform -and $platform -ne 'x64') {
        throw "The existing native build targets $platform, not x64. Use a separate x64 build directory."
    }
} else {
    $configure += @('-A', 'x64')
}
& $cmake @configure
if ($LASTEXITCODE -ne 0) { throw "Native CMake configuration failed ($LASTEXITCODE)." }
& $cmake --build $build --config Debug
if ($LASTEXITCODE -ne 0) { throw "Native compilation failed ($LASTEXITCODE)." }
& $ctest --test-dir $build -C Debug --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "Native tests failed ($LASTEXITCODE)." }
