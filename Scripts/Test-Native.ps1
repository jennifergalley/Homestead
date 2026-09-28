<#
.SYNOPSIS
Builds and runs the native simulation test suites (CMake + CTest, no Unreal).
.DESCRIPTION
cmake and ctest aren't on PATH; this script finds Visual Studio's copies with vswhere
(<VS>\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin). Use the same path to run cmake yourself.
Debug (the default) keeps assertions but the simulation suite takes about 10 minutes. -Configuration Release
runs every suite in about 3 minutes; use it for routine checks. To run one suite directly, build it and
redirect its output to a file (stdout is buffered, so a crash loses unredirected output):
  cmake --build Build\Native --config Release --target HomesteadSimulationTests
  Build\Native\Release\HomesteadSimulationTests.exe *> Build\Native\sim-tests.log
#>
[CmdletBinding()]
param([ValidateSet('Debug','Release')][string]$Configuration = 'Debug')
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
& $cmake --build $build --config $Configuration
if ($LASTEXITCODE -ne 0) { throw "Native compilation failed ($LASTEXITCODE)." }
& $ctest --test-dir $build -C $Configuration --output-on-failure --timeout 1800
if ($LASTEXITCODE -ne 0) { throw "Native tests failed ($LASTEXITCODE)." }
