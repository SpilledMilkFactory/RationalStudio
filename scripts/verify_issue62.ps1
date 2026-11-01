#requires -Version 5.1
<#
Issue #62 — native Windows x64 portable ZIP qualification.
Must be run on Windows with Visual Studio 2022 Build Tools, Qt6 MSVC kit,
Boost headers, CMake, Git and Python 3 available. Does NOT modify the source.
Creates a FRESH Release build, tests 73 CTests, installs, deploys Qt,
copies the approved MSVC app-local redist directory, zips, audits and smoke-tests.
Official clean Windows install/launch and legal sign-off are separate gates.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$QtRoot,
    [Parameter(Mandatory=$true)][string]$BoostInclude,
    [string]$SourceRoot = '',
    [string]$BuildRoot = '',
    [string]$EvidenceRoot = '',
    [string]$MSVCRedistDir = '',
    [ValidateRange(1,32)][int]$Jobs = 4
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
# Resolve the repository root after parameter binding is complete.
if ([string]::IsNullOrWhiteSpace($SourceRoot)) {
    $SourceRoot = Split-Path -Path $PSScriptRoot -Parent
}
$SourceRoot = (Resolve-Path -LiteralPath $SourceRoot).Path
$SourceRoot = (Resolve-Path -LiteralPath $SourceRoot).Path
$QtRoot = [IO.Path]::GetFullPath($QtRoot)
$BoostInclude = [IO.Path]::GetFullPath($BoostInclude)
if (-not $BuildRoot) { $BuildRoot = Join-Path $SourceRoot 'build-issue62-windows' }
if (-not $EvidenceRoot) { $EvidenceRoot = Join-Path $SourceRoot 'issue62-evidence' }
$BuildRoot = [IO.Path]::GetFullPath($BuildRoot)
$EvidenceRoot = [IO.Path]::GetFullPath($EvidenceRoot)

function Assert-File([string]$File, [string]$Purpose) {
    if (-not (Test-Path -LiteralPath $File -PathType Leaf)) { throw "$Purpose missing: $File" }
}
function Need-Command([string]$Name) {
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) { throw "Required command unavailable: $Name" }
}
function Invoke-Logged([string]$Name, [string]$Exe, [string[]]$Arguments) {
    $log = Join-Path $EvidenceRoot ($Name + '.log')
    Write-Host "===== $Name ====="
    $oldPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        & $Exe @Arguments 2>&1 | Tee-Object -FilePath $log | Out-Host
        $status = $LASTEXITCODE
    } finally { $ErrorActionPreference = $oldPreference }
    if ($status -ne 0) { throw "$Name failed ($status): $log" }
}
function Find-Redistributable([string]$Explicit) {
    if ($Explicit) { return [IO.Path]::GetFullPath($Explicit) }
    $search = @()
    if ($env:VCToolsRedistDir) {
        $search += [IO.Path]::GetFullPath($env:VCToolsRedistDir)
    }
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path -LiteralPath $vswhere) {
        $install = (& $vswhere -latest -products '*' -property installationPath | Select-Object -First 1)
        if ($install) { $search += (Join-Path $install 'VC\Redist\MSVC') }
    }
    $found = @()
    foreach ($base in $search) {
        if (-not (Test-Path -LiteralPath $base -PathType Container)) { continue }
        $found += @(Get-ChildItem -LiteralPath $base -Directory -Recurse -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -eq 'Microsoft.VC143.CRT' -and $_.Parent.Name -eq 'x64' })
    }
    $best = $found | Sort-Object FullName -Descending | Select-Object -First 1
    if (-not $best) {
        throw 'Official MSVC Redistributable CRT directory not found. Provide -MSVCRedistDir pointing to ...\VC\Redist\MSVC\<version>\x64\Microsoft.VC143.CRT'
    }
    return $best.FullName
}

Assert-File (Join-Path $SourceRoot 'CMakeLists.txt') 'Project CMakeLists'
Assert-File (Join-Path $QtRoot 'bin\windeployqt.exe') 'Qt deployment tool'
Assert-File (Join-Path $QtRoot 'bin\Qt6Core.dll') 'Qt MSVC Release runtime'
Assert-File (Join-Path $BoostInclude 'boost\multiprecision\cpp_int.hpp') 'Boost headers'
Assert-File (Join-Path $SourceRoot 'scripts\packaging\check_windows_zip.py') 'Portable ZIP validator'
if ($QtRoot -notmatch '(?i)msvc2022_64') { throw 'QtRoot must be an MSVC 2022 64-bit Qt kit' }
foreach ($exe in @('cmake.exe','ctest.exe','python.exe','git.exe')) { Need-Command $exe }
if (Test-Path -LiteralPath $BuildRoot) { throw "Refusing to overwrite build: $BuildRoot" }
if (Test-Path -LiteralPath $EvidenceRoot) { throw "Refusing to overwrite evidence: $EvidenceRoot" }

$MSVCRedistDir = Find-Redistributable $MSVCRedistDir
if ($MSVCRedistDir -notmatch '(?i)[\\/]VC[\\/]Redist[\\/]MSVC[\\/].+[\\/]x64[\\/]Microsoft\.VC143\.CRT$') {
    throw "Redist path is NOT the official MSVC x64 redistributable CRT directory: $MSVCRedistDir"
}
Assert-File (Join-Path $MSVCRedistDir 'vcruntime140.dll') 'Redistributable CRT'
Assert-File (Join-Path $MSVCRedistDir 'msvcp140.dll') 'Redistributable C++ library'
$versionMatch = [regex]::Match((Get-Content -LiteralPath (Join-Path $SourceRoot 'CMakeLists.txt') -Raw),
    'project\(RationalStudio\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)')
if (-not $versionMatch.Success) { throw 'Cannot determine project version from CMakeLists.txt' }
$version = $versionMatch.Groups[1].Value
$packageName = "RationalStudio-$version-Windows-x64"
$packageDir = Join-Path $EvidenceRoot 'packages'
$stageRoot = Join-Path $EvidenceRoot "staging\$packageName"
$zipPath = Join-Path $packageDir ($packageName + '.zip')
$extractRoot = Join-Path $EvidenceRoot 'extracted'
New-Item -ItemType Directory -Path $EvidenceRoot -Force | Out-Null
New-Item -ItemType Directory -Path $packageDir -Force | Out-Null
$head = (& git.exe -C $SourceRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Git HEAD unavailable' }
@(
    "Issue #62 Windows portable Release qualification",
    "Git revision: $head",
    "Host: $([Environment]::OSVersion.VersionString)",
    "Qt: $QtRoot",
    "Boost: $BoostInclude",
    "MSVC official redistributable: $MSVCRedistDir",
    "Project version: $version",
    "Date UTC: $([DateTime]::UtcNow.ToString('o'))",
    "Known limitation: 125% Windows display scaling layout; separately tracked",
    "Legal gate: Qt corresponding source/relinking review pending"
) | Set-Content -LiteralPath (Join-Path $EvidenceRoot 'environment.txt') -Encoding UTF8

$env:PATH = (Join-Path $QtRoot 'bin') + ';' + $env:PATH
Invoke-Logged 'configure' 'cmake.exe' @(
    '-S', $SourceRoot, '-B', $BuildRoot, '-G', 'Visual Studio 17 2022', '-A', 'x64',
    "-DCMAKE_PREFIX_PATH=$QtRoot", "-DRATIONALSTUDIO_BOOST_INCLUDE_DIR=$BoostInclude",
    '-DRATIONALSTUDIO_BUILD_GUI=ON', '-DRATIONALSTUDIO_BUILD_CLI=ON', '-DBUILD_TESTING=ON',
    '-DRATIONALSTUDIO_BUILD_BENCHMARKS=OFF',
    '-DRATIONALSTUDIO_BUILD_ADVANCED_GRAPH_BENCHMARKS=OFF',
    '-DRATIONALSTUDIO_ENABLE_LINUX_PACKAGING=OFF',
    '-DRATIONALSTUDIO_ENABLE_WINDOWS_PACKAGING=ON'
)
Invoke-Logged 'release-build' 'cmake.exe' @('--build',$BuildRoot,'--config','Release','--parallel',"$Jobs")
$inventoryPath = Join-Path $EvidenceRoot 'ctest-inventory.json'
& ctest.exe --test-dir $BuildRoot -C Release --show-only=json-v1 |
    Out-File -FilePath $inventoryPath -Encoding utf8
if ($LASTEXITCODE -ne 0) { throw 'CTest inventory failed' }
$inventory = Get-Content -LiteralPath $inventoryPath -Raw | ConvertFrom-Json
if (@($inventory.tests).Count -ne 73) { throw "Expected 73 CTests; got $(@($inventory.tests).Count)" }
Invoke-Logged 'release-ctest' 'ctest.exe' @('--test-dir',$BuildRoot,'-C','Release','--output-on-failure','--parallel',"$Jobs",'--no-tests=error')
Assert-File (Join-Path $BuildRoot 'Release\RationalStudio.exe') 'Release desktop executable'
Assert-File (Join-Path $BuildRoot 'Release\RationalCLI.exe') 'Release CLI executable'

Invoke-Logged 'install' 'cmake.exe' @('--install',$BuildRoot,'--config','Release','--prefix',$stageRoot)
$bin = Join-Path $stageRoot 'bin'
Assert-File (Join-Path $bin 'RationalStudio.exe') 'Staged GUI executable'
Invoke-Logged 'windeployqt' (Join-Path $QtRoot 'bin\windeployqt.exe') @(
    '--release','--no-compiler-runtime','--no-translations','--dir',$bin,(Join-Path $bin 'RationalStudio.exe')
)
Assert-File (Join-Path $bin 'platforms\qwindows.dll') 'Qt Windows platform plugin'
# Pull only redistributable DLLs from the official MSVC app-local directory.
# Do not use /MT, system32 files, compiler tools DLLs, debug_nonredist, etc.
$crtDlls = @(Get-ChildItem -LiteralPath $MSVCRedistDir -File -Filter '*.dll')
if ($crtDlls.Count -lt 2) { throw 'Official MSVC redistributable directory is unexpectedly empty' }
foreach ($dll in $crtDlls) {
    if ($dll.Name -match '(?i)d\.dll$|debug') { throw "Debug CRT DLL prohibited: $($dll.Name)" }
    Copy-Item -LiteralPath $dll.FullName -Destination (Join-Path $bin $dll.Name) -Force
}
# Move the executable, required Qt DLLs/plugins and CRT beside each other so
# users can extract the archive and double-click RationalStudio.exe directly.
Get-ChildItem -LiteralPath $bin -Force | ForEach-Object {
    $target = Join-Path $stageRoot $_.Name
    if (Test-Path -LiteralPath $target) { throw "Staging collision: $target" }
    Move-Item -LiteralPath $_.FullName -Destination $stageRoot
}
Remove-Item -LiteralPath $bin -Force
@('[Paths]','Prefix=.','Plugins=.') |
    Set-Content -LiteralPath (Join-Path $stageRoot 'qt.conf') -Encoding Ascii
Assert-File (Join-Path $stageRoot 'platforms\qwindows.dll') 'Relocated Windows platform plugin'

$hashFile = Join-Path $stageRoot 'SHA256SUMS.txt'
$lines = @()
Get-ChildItem -LiteralPath $stageRoot -Recurse -File | Sort-Object FullName | ForEach-Object {
    if ($_.FullName -eq $hashFile) { return }
    $rel = $_.FullName.Substring($stageRoot.Length + 1).Replace('\','/')
    $hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    $lines += "$hash  $rel"
}
$lines | Set-Content -LiteralPath $hashFile -Encoding Ascii

# Compress the whole named directory, not merely its contents.
Compress-Archive -LiteralPath $stageRoot -DestinationPath $zipPath -CompressionLevel Optimal
Assert-File $zipPath 'Portable release ZIP'
Invoke-Logged 'audit-zip' 'python.exe' @(
    (Join-Path $SourceRoot 'scripts\packaging\check_windows_zip.py'),
    '--zip',$zipPath,'--source',$SourceRoot,'--version',$version
)
$zipSha = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
"$zipSha  $($packageName).zip" |
    Set-Content -LiteralPath (Join-Path $packageDir ($packageName + '.zip.sha256')) -Encoding Ascii

# Extract into a different directory, remove Qt SDK from PATH and check that
# the archive's own Qt and Microsoft runtimes can initialize the real process.
Expand-Archive -LiteralPath $zipPath -DestinationPath $extractRoot
$extracted = Join-Path $extractRoot $packageName
Assert-File (Join-Path $extracted 'RationalStudio.exe') 'Extracted GUI executable'
$oldPath = $env:PATH
$oldPlatform = $env:QT_QPA_PLATFORM
$oldPluginPath = $env:QT_PLUGIN_PATH
$oldQpaPluginPath = $env:QT_QPA_PLATFORM_PLUGIN_PATH
try {
    $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot;$env:SystemRoot\System32\Wbem"
    $env:QT_QPA_PLATFORM = 'windows'
    $env:QT_PLUGIN_PATH = ''
    $env:QT_QPA_PLATFORM_PLUGIN_PATH = ''
    $proc = Start-Process -FilePath (Join-Path $extracted 'RationalStudio.exe') `
        -WorkingDirectory $extracted -PassThru
    try {
        Start-Sleep -Seconds 4
        $proc.Refresh()
        if ($proc.HasExited) { throw "Portable desktop terminated prematurely; exit=$($proc.ExitCode)" }
    } finally {
        if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue }
        $proc.Dispose()
    }
    'PASS: extracted GUI initializes without Qt SDK directories on PATH (same host)' |
        Add-Content -LiteralPath (Join-Path $EvidenceRoot 'results.txt')
    # End-user CLI also needs the redistributable runtime but does not need Qt.
    $start = New-Object System.Diagnostics.ProcessStartInfo
    $start.FileName = Join-Path $extracted 'RationalCLI.exe'
    $start.WorkingDirectory = $extracted
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.RedirectStandardInput = $true
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    $cli = New-Object System.Diagnostics.Process
    $cli.StartInfo = $start
    try {
        if (-not $cli.Start()) { throw 'Portable CLI did not start' }
        $cli.StandardInput.WriteLine('1/2 1/3')
        $cli.StandardInput.WriteLine('+')
        $cli.StandardInput.Close()
        $cliOutput = $cli.StandardOutput.ReadToEnd()
        $cliError = $cli.StandardError.ReadToEnd()
        if (-not $cli.WaitForExit(10000)) {
            $cli.Kill()
            throw 'Portable CLI timed out'
        }
        if ($cli.ExitCode -ne 0 -or $cliOutput -notmatch '1/2 \+ 1/3 = 5/6') {
            throw "Portable CLI calculation failed: exit=$($cli.ExitCode), stdout=$cliOutput, stderr=$cliError"
        }
    } finally { $cli.Dispose() }
    'PASS: extracted CLI calculates 1/2 + 1/3 = 5/6 without Qt SDK on PATH (same host)' |
        Add-Content -LiteralPath (Join-Path $EvidenceRoot 'results.txt')
} finally {
    $env:PATH = $oldPath
    $env:QT_QPA_PLATFORM = $oldPlatform
    $env:QT_PLUGIN_PATH = $oldPluginPath
    $env:QT_QPA_PLATFORM_PLUGIN_PATH = $oldQpaPluginPath
}
'PASS: Windows Release build and 73/73 CTests; Qt+MSVC app-local staging; audited portable ZIP' |
    Add-Content -LiteralPath (Join-Path $EvidenceRoot 'results.txt')
Write-Host "PASS: Issue #62 Windows portable package qualified on build host: $zipPath"
Write-Host "SHA256: $zipSha"
Write-Host 'Still REQUIRED: clean Windows 10/11 machine with NO Qt SDK and NO MSVC Build Tools; manual GUI/CLI acceptance; Qt license/source review.'
