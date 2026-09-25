# Builds (unless -NoBuild) and runs Voyager-2 from the project root.
#
# The working directory matters: shaders are loaded from "shaders/..."
# by relative path, so the exe must be started from the project root, not from
# x64\Debug. That is why this script exists instead of double-clicking the exe.
#
#   .\scripts\run.ps1
#   .\scripts\run.ps1 -Configuration Release
#   .\scripts\run.ps1 -NoBuild

[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',

    [ValidateSet('x64', 'Win32')]
    [string]$Platform = 'x64',

    [switch]$NoBuild
)

$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot

if (-not $NoBuild) {
    & (Join-Path $PSScriptRoot 'build.ps1') -Configuration $Configuration -Platform $Platform
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$exe = Join-Path $projectRoot "$Platform\$Configuration\Voyager-2.exe"
if (-not (Test-Path $exe)) {
    Write-Error "Executable not found: $exe. Build first."
    exit 1
}

Write-Host "[RUN] $exe (cwd: $projectRoot)" -ForegroundColor Cyan

Push-Location $projectRoot
try {
    & $exe
    $code = $LASTEXITCODE
}
finally {
    Pop-Location
}

Write-Host "[RUN] exited with $code" -ForegroundColor DarkGray
exit $code
