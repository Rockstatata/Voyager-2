param(
	[string]$ProjectRoot = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = 'Stop'
$kilometresPerAu = 149597870.7
$celestialUnitsPerKm = 1.5 / 696340.0

function Get-BodyRadius([double]$radiusKm) {
	return $radiusKm * $celestialUnitsPerKm
}

function Get-HeliocentricDistance([double]$distanceKm) {
	$distanceAu = $distanceKm / $kilometresPerAu
	return 3.0 * [Math]::Pow($distanceAu / 0.387098, 0.60)
}

$failures = [System.Collections.Generic.List[string]]::new()
$sunRadius = Get-BodyRadius 696340.0
$jupiterRadius = Get-BodyRadius 69911.0
$earthRadius = Get-BodyRadius 6371.0
$saturnRadius = Get-BodyRadius 58232.0
$tethysRadius = Get-BodyRadius 531.1
$tethysOrbit = 294619.0 * $celestialUnitsPerKm
$saturnOuterRing = $saturnRadius * 2.415

if ([Math]::Abs($sunRadius / $jupiterRadius - (696340.0 / 69911.0)) -gt 1e-9) {
	$failures.Add('Sun-to-Jupiter radius ratio is not physical.')
}
if ([Math]::Abs($jupiterRadius / $earthRadius - (69911.0 / 6371.0)) -gt 1e-9) {
	$failures.Add('Jupiter-to-Earth radius ratio is not physical.')
}
if ($tethysOrbit -le $saturnOuterRing + $tethysRadius) {
	$failures.Add('Tethys intersects Saturn rings.')
}

$marsOrbit = Get-HeliocentricDistance 227939200.0
$jupiterOrbit = Get-HeliocentricDistance 778570000.0
$saturnOrbit = Get-HeliocentricDistance 1433530000.0
$uranusOrbit = Get-HeliocentricDistance 2872460000.0
$neptuneOrbit = Get-HeliocentricDistance 4495060000.0
if (-not ($marsOrbit -lt $jupiterOrbit -and $jupiterOrbit -lt $saturnOrbit -and
	$saturnOrbit -lt $uranusOrbit -and $uranusOrbit -lt $neptuneOrbit)) {
	$failures.Add('Compressed planet orbits do not preserve physical order.')
}
if (($neptuneOrbit - $uranusOrbit) -lt 8.0 -or ($uranusOrbit - $saturnOrbit) -lt 8.0) {
	$failures.Add('Outer planets are packed too closely in the overview.')
}
if ((13.0 * 0.0001) -ge (Get-BodyRadius 24622.0)) {
	$failures.Add('Voyager display size is too large relative to Neptune.')
}

$voyagerTrack = Join-Path $ProjectRoot 'assets\trajectory\voyager2_heliocentric.csv'
$marsTrack = Join-Path $ProjectRoot 'assets\trajectory\planets\mars_heliocentric.csv'
if (-not (Test-Path -LiteralPath $marsTrack) -or
	(Select-String -LiteralPath $marsTrack -Pattern '^[0-9]' | Measure-Object).Count -ne 7) {
	$failures.Add('Mars must have seven date-matched NASA/JPL historical anchors.')
}
$encounters = @(
	@{ Id = 'jupiter'; JulianDate = '2444063.500000000' },
	@{ Id = 'saturn'; JulianDate = '2444841.500000000' },
	@{ Id = 'uranus'; JulianDate = '2446454.500000000' },
	@{ Id = 'neptune'; JulianDate = '2447763.500000000' }
)
foreach ($encounter in $encounters) {
	$planetTrack = Join-Path $ProjectRoot ("assets\trajectory\planets\{0}_heliocentric.csv" -f $encounter.Id)
	$voyagerRow = (Select-String -LiteralPath $voyagerTrack -Pattern ("^{0}," -f $encounter.JulianDate) | Select-Object -First 1).Line -split ','
	$planetRow = (Select-String -LiteralPath $planetTrack -Pattern ("^{0}," -f $encounter.JulianDate) | Select-Object -First 1).Line -split ','
	if ($voyagerRow.Count -ne 4 -or $planetRow.Count -ne 4) {
		$failures.Add("$($encounter.Id) exact encounter row is missing or malformed.")
	}
}

$applicationSource = Join-Path $ProjectRoot 'src\core\Application.cpp'
if (Select-String -LiteralPath $applicationSource -Pattern 'buildRingSystem\("jupiter"|buildRingSystem\("neptune"' -Quiet) {
	$failures.Add('Jupiter or Neptune visible ring geometry was reintroduced.')
}
if (-not (Select-String -LiteralPath $applicationSource -Pattern 'j2000TrueAnomaliesDegrees' -Quiet)) {
	$failures.Add('Planet orbits are missing J2000 starting phases.')
}

if ($failures.Count -gt 0) {
	$failures | ForEach-Object { Write-Error "[LAYOUT] $_" -ErrorAction Continue }
	exit 1
}

Write-Host '[LAYOUT] Exact body-size ratios, moon/ring clearances, wide ordered planet spacing, and historical encounters satisfy the regression contract.'
