#requires -Version 5.1
<#
Rational Studio — Issue #60 native Windows x64 qualification.
Run from "x64 Native Tools Command Prompt for VS 2022" with:
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\verify_issue60.ps1 `
    -QtRoot "C:\Qt\6.10.2\msvc2022_64" `
    -BoostInclude "C:\dev\vcpkg\installed\x64-windows\include"
The script DOES NOT install software or modify application source.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$QtRoot,
    [Parameter(Mandatory = $true)][string]$BoostInclude,
    [string]$SourceRoot = (Split-Path $PSScriptRoot -Parent),
    [string]$BuildRoot = '',
    [string]$EvidenceRoot = '',
    [ValidateRange(1, 64)][int]$Jobs = 4
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$SourceRoot = (Resolve-Path -LiteralPath $SourceRoot).Path
if (-not $BuildRoot)    { $BuildRoot = Join-Path $SourceRoot 'build-issue60-windows' }
if (-not $EvidenceRoot) { $EvidenceRoot = Join-Path $SourceRoot 'issue60-windows-evidence' }
$QtRoot = [IO.Path]::GetFullPath($QtRoot)
$BoostInclude = [IO.Path]::GetFullPath($BoostInclude)
$BuildRoot = [IO.Path]::GetFullPath($BuildRoot)
$EvidenceRoot = [IO.Path]::GetFullPath($EvidenceRoot)

function Assert-File([string]$Path, [string]$Purpose) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Purpose missing: $Path"
    }
}
function Assert-Command([string]$Name) {
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Required command not in PATH: $Name. Start the VS 2022 x64 Native Tools command prompt."
    }
}
function Invoke-Logged([string]$Label, [string]$Executable, [string[]]$Arguments) {
    $safe = $Label -replace '[^A-Za-z0-9_-]', '_'
    $log = Join-Path $EvidenceRoot ($safe + '.log')
    Write-Host "===== $Label ====="
    # Native-command non-zero exit codes are checked explicitly, and logs are retained.
    $savedPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        & $Executable @Arguments 2>&1 | Tee-Object -FilePath $log | Out-Host
        $code = $LASTEXITCODE
    } finally { $ErrorActionPreference = $savedPreference }
    if ($code -ne 0) { throw "$Label failed with exit code $code. See $log" }
    return $log
}

Assert-File (Join-Path $SourceRoot 'CMakeLists.txt') 'Rational Studio source'
Assert-File (Join-Path $QtRoot 'lib\cmake\Qt6\Qt6Config.cmake') 'Qt MSVC x64 CMake package'
Assert-File (Join-Path $QtRoot 'bin\Qt6Core.dll') 'Qt runtime'
Assert-File (Join-Path $BoostInclude 'boost\multiprecision\cpp_int.hpp') 'Boost.Multiprecision header'
foreach ($command in @('cmake.exe', 'ctest.exe', 'ninja.exe', 'cl.exe', 'git.exe')) {
    Assert-Command $command
}
if (Test-Path -LiteralPath $BuildRoot) { throw "Build directory exists: $BuildRoot (use a fresh -BuildRoot)" }
if (Test-Path -LiteralPath $EvidenceRoot) { throw "Evidence directory exists: $EvidenceRoot (use a fresh -EvidenceRoot)" }
New-Item -ItemType Directory -Path $EvidenceRoot -Force | Out-Null

$env:PATH = (Join-Path $QtRoot 'bin') + ';' + $env:PATH
$sourceHashBefore = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $SourceRoot 'CMakeLists.txt')).Hash
$revision = (& git.exe -C $SourceRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Cannot read Git revision' }
$versionOutput = @(
    "Source: $SourceRoot",
    "Commit: $revision",
    "Qt root: $QtRoot",
    "Boost include: $BoostInclude",
    "Windows: $([Environment]::OSVersion.VersionString)",
    "Architecture: $env:PROCESSOR_ARCHITECTURE",
    "Date (UTC): $([DateTime]::UtcNow.ToString('o'))"
)
$versionOutput | Set-Content -Encoding UTF8 -Path (Join-Path $EvidenceRoot 'environment.txt')
& cmake.exe --version | Out-File -Encoding UTF8 (Join-Path $EvidenceRoot 'cmake-version.txt')
& ninja.exe --version | Out-File -Encoding UTF8 (Join-Path $EvidenceRoot 'ninja-version.txt')

foreach ($configuration in @('Debug', 'Release')) {
    $build = Join-Path $BuildRoot $configuration.ToLowerInvariant()
    $configureArgs = @(
        '-S', $SourceRoot, '-B', $build, '-G', 'Ninja',
        "-DCMAKE_BUILD_TYPE=$configuration", '-DBUILD_TESTING=ON',
        '-DRATIONALSTUDIO_BUILD_GUI=ON', '-DRATIONALSTUDIO_BUILD_CLI=ON',
        '-DRATIONALSTUDIO_BUILD_BENCHMARKS=OFF',
        '-DRATIONALSTUDIO_BUILD_ADVANCED_GRAPH_BENCHMARKS=OFF',
        '-DRATIONALSTUDIO_ENABLE_LINUX_PACKAGING=OFF',
        "-DCMAKE_PREFIX_PATH=$QtRoot",
        "-DRATIONALSTUDIO_BOOST_INCLUDE_DIR=$BoostInclude"
    )
    Invoke-Logged "${configuration}-configure" 'cmake.exe' $configureArgs | Out-Null
    Invoke-Logged "${configuration}-build" 'cmake.exe' @('--build', $build, '--parallel', "$Jobs") | Out-Null

    $inventoryPath = Join-Path $EvidenceRoot "${configuration}-test-inventory.json"
    & ctest.exe --test-dir $build --show-only=json-v1 | Out-File -Encoding UTF8 $inventoryPath
    if ($LASTEXITCODE -ne 0) { throw "Could not inspect $configuration CTest inventory" }
    $inventory = Get-Content -Raw -LiteralPath $inventoryPath | ConvertFrom-Json
    $count = @($inventory.tests).Count
    if ($count -ne 73) { throw "$configuration: expected 73 GUI CTests, found $count" }

    Invoke-Logged "${configuration}-ctest" 'ctest.exe' @(
        '--test-dir', $build, '--output-on-failure', '--no-tests=error'
    ) | Out-Null

    Assert-File (Join-Path $build 'RationalStudio.exe') "$configuration GUI executable"
    Assert-File (Join-Path $build 'RationalCLI.exe') "$configuration CLI executable"
    "PASS: $configuration 73/73 CTests, GUI/CLI binaries present" |
        Add-Content -Encoding UTF8 -Path (Join-Path $EvidenceRoot 'results.txt')
}

$sourceHashAfter = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $SourceRoot 'CMakeLists.txt')).Hash
if ($sourceHashBefore -ne $sourceHashAfter) { throw 'CMakeLists.txt changed during qualification' }
'PASS: CMakeLists source checksum stable' |
    Add-Content -Encoding UTF8 -Path (Join-Path $EvidenceRoot 'results.txt')
Write-Host 'PASS: Issue #60 Windows Debug and Release CMake/CTest qualification complete.'
Write-Host "Evidence: $EvidenceRoot"
Write-Host 'Manual GUI checks and toolchain compatibility review are still required.'
