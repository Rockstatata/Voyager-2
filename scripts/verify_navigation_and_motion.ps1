param(
	[string]$ProjectRoot = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = 'Stop'
$application = Get-Content -Raw (Join-Path $ProjectRoot 'src\core\Application.cpp')
$cameraHeader = Get-Content -Raw (Join-Path $ProjectRoot 'src\rendering\Camera.h')
$voyager = Get-Content -Raw (Join-Path $ProjectRoot 'src\scene\Voyager2.cpp')
$scale = Get-Content -Raw (Join-Path $ProjectRoot 'src\scene\ScaleManager.cpp')
$failures = [System.Collections.Generic.List[string]]::new()

if ($voyager -match 'std::fmod\(') {
	$failures.Add('Historical playback still wraps to launch and teleports moving bodies.')
}
if ($application -match '_historical_orbit') {
	$failures.Add('Sparse historical planet-path lines are still rendered into the overview.')
}
if ($application -match 'bool m_trajectoryVisible = true') {
	$failures.Add('The long Voyager trajectory is still shown by default.')
}
if ($cameraHeader -notmatch 'orbitFollowTarget') {
	$failures.Add('Third-person camera has no mouse-driven orbit-follow method.')
}
if ($application -notmatch 'm_starfield->transform\(\)\.position = m_camera\.position\(\)') {
	$failures.Add('Starfield does not follow the active camera.')
}
if ($scale -match 'return semiMajorAxisKm \* kCelestialUnitsPerKm;') {
	$failures.Add('Moon presentation distance is still the uncompressed raw physical distance.')
}

if ($failures.Count -gt 0) {
	$failures | ForEach-Object { Write-Error "[NAVIGATION] $_" -ErrorAction Continue }
	exit 1
}

Write-Host '[NAVIGATION] Continuous historical time, uncluttered default scene, mouse orbit-follow, camera-centred stars, and compact moon display distances satisfy the regression contract.'
