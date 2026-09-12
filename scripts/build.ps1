# Builds Voyager-2 without needing a Developer PowerShell prompt.
#
# vswhere reports where Visual Studio is installed, so MSBuild is called by its
# full path. That is the whole reason `code .` can be launched from an ordinary
# terminal (or the VS Code icon) instead of "Developer PowerShell for VS".
#
#   .\scripts\build.ps1                     # Debug x64
#   .\scripts\build.ps1 -Configuration Release
#   .\scripts\build.ps1 -Rebuild

[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',

    [ValidateSet('x64', 'Win32')]
    [string]$Platform = 'x64',

    [switch]$Rebuild
)

$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$project = Join-Path $projectRoot 'Voyager-2.vcxproj'

if (-not (Test-Path $project)) {
    Write-Error "Project file not found: $project"
    exit 1
}

function Find-MSBuild {
    # Already on PATH (a Developer prompt, or a manual PATH entry).
    $onPath = Get-Command msbuild.exe -ErrorAction SilentlyContinue
    if ($onPath) { return $onPath.Source }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $found = & $vswhere -latest -prerelease -products * `
            -requires Microsoft.Component.MSBuild `
            -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
        if ($found) { return $found }
    }

    return $null
}

$msbuild = Find-MSBuild
if (-not $msbuild) {
    Write-Error @'
MSBuild was not found.

Install the "Desktop development with C++" workload from the Visual Studio
Installer, which provides MSBuild, the MSVC compiler and the Windows SDK.
'@
    exit 1
}

$target = if ($Rebuild) { 'Rebuild' } else { 'Build' }

Write-Host "[BUILD] $target $Configuration|$Platform" -ForegroundColor Cyan
Write-Host "[BUILD] msbuild: $msbuild" -ForegroundColor DarkGray

& $msbuild $project `
    /nologo `
    /m `
    /v:minimal `
    /t:$target `
    /p:Configuration=$Configuration `
    /p:Platform=$Platform

$code = $LASTEXITCODE
if ($code -ne 0) {
    Write-Host "[BUILD] failed (exit $code)" -ForegroundColor Red
    exit $code
}

$exe = Join-Path $projectRoot "$Platform\$Configuration\Voyager-2.exe"
Write-Host "[BUILD] ok -> $exe" -ForegroundColor Green
