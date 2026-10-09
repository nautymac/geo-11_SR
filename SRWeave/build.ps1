# Build bin\x64\dxgi.dll and/or bin\x86\dxgi.dll (Release) with the CMake bundled in Visual Studio.
#   build.ps1              -> both architectures
#   build.ps1 -Arch x64    -> 64-bit only (build\)
#   build.ps1 -Arch x86    -> 32-bit only (build-x86\)
# Extra arguments go to the CMake configure step, e.g. build.ps1 -Arch x86 -DSR_SDK=<path>.
param(
    [ValidateSet('x64', 'x86', 'all')] [string] $Arch = 'all',
    [Parameter(ValueFromRemainingArguments = $true)] [string[]] $CMakeArgs
)
$ErrorActionPreference = 'Stop'
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio with C++ tools not found' }
$cmake = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$root = $PSScriptRoot

$targets = @()
if ($Arch -eq 'all' -or $Arch -eq 'x64') { $targets += @{ platform = 'x64';   dir = (Join-Path $root 'build') } }
if ($Arch -eq 'all' -or $Arch -eq 'x86') { $targets += @{ platform = 'Win32'; dir = (Join-Path $root 'build-x86') } }

foreach ($t in $targets) {
    Write-Host "=== configure $($t.platform) ===" -ForegroundColor Cyan
    & $cmake -S $root -B $t.dir -A $t.platform @CMakeArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Write-Host "=== build $($t.platform) ===" -ForegroundColor Cyan
    & $cmake --build $t.dir --config Release
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
Get-ChildItem (Join-Path $root 'bin') -Recurse -Filter dxgi.dll | ForEach-Object { Write-Host "built: $($_.FullName) ($($_.Length) bytes)" }
