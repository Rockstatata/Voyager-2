param(
	[string]$ProjectRoot = (Split-Path -Parent $PSScriptRoot)
)

# Offline regression check for the presentation scale, ring/moon clearances,
# the Horizons data files and the four historical flyby geometries.
# Mirrors ScaleManager and MissionEphemeris; no build or GPU needed.

$ErrorActionPreference = 'Stop'
$culture = [System.Globalization.CultureInfo]::InvariantCulture
$kilometresPerAu = 149597870.7
$failures = [System.Collections.Generic.List[string]]::new()

function Get-BodyRadius([double]$radiusKm) {
	return 0.50 * [Math]::Pow($radiusKm / 6371.0, 0.60)
}

function Get-HeliocentricDistance([double]$distanceAu) {
	return 12.0 * [Math]::Pow($distanceAu / 0.387098, 0.55)
}

function Get-MoonOrbit([double]$semiMajorAxisKm, [double]$parentRadiusKm) {
	return 1.6 + [Math]::Sqrt($semiMajorAxisKm / $parentRadiusKm) # parent radii
}

# --- Body size order survives compression; the Sun stays largest. ---
$sunRadius = 6.0
$ordered = @(69911.0, 58232.0, 25362.0, 24622.0, 6371.0, 6051.8, 3389.5, 2634.1, 2574.7, 2439.7, 1821.6, 1737.4, 1188.3, 235.8)
for ($i = 1; $i -lt $ordered.Count; $i++) {
	if ((Get-BodyRadius $ordered[$i]) -ge (Get-BodyRadius $ordered[$i - 1])) {
		$failures.Add("Radius order broken at $($ordered[$i]) km.")
	}
}
if ($sunRadius -le 2.0 * (Get-BodyRadius 69911.0)) {
	$failures.Add('The Sun is no longer clearly the largest body.')
}
if ($sunRadius -ge (Get-HeliocentricDistance 0.307)) {
	$failures.Add('The Sun swallows Mercury at perihelion.')
}
if ((Get-BodyRadius 6371.0) -lt 0.3) {
	$failures.Add('Earth is too small to read in the overview.')
}

# --- Moons clear every ring system. ---
$innerMoons = @(
	@{ Name = 'Tethys'; Orbit = (Get-MoonOrbit 294619.0 58232.0); Ring = 2.334 },
	@{ Name = 'Miranda'; Orbit = (Get-MoonOrbit 129900.0 25362.0); Ring = 2.020 },
	@{ Name = 'Io'; Orbit = (Get-MoonOrbit 421700.0 69911.0); Ring = 1.81 },
	@{ Name = 'Triton'; Orbit = (Get-MoonOrbit 354800.0 24622.0); Ring = 2.55 }
)
foreach ($moon in $innerMoons) {
	if ($moon.Orbit -le $moon.Ring + 0.5) {
		$failures.Add("$($moon.Name) orbit ($($moon.Orbit)) intersects its planet's rings.")
	}
}

# --- Planet spacing: order kept, outer planets not packed together. ---
$axes = @(0.387, 0.723, 1.0, 1.524, 5.204, 9.583, 19.218, 30.07) | ForEach-Object { Get-HeliocentricDistance $_ }
for ($i = 1; $i -lt $axes.Count; $i++) {
	if ($axes[$i] -le $axes[$i - 1] + 2.0) {
		$failures.Add("Planet orbits $i and $($i - 1) are too close in the overview.")
	}
}
if (($axes[7] - $axes[6]) -lt 15.0 -or ($axes[6] - $axes[5]) -lt 15.0) {
	$failures.Add('Outer planets are packed too closely in the overview.')
}
if ((13.0 * 0.002) -ge (Get-BodyRadius 24622.0) * 0.1) {
	$failures.Add('Voyager display size is too large relative to Neptune.')
}

# --- Horizons tables. ---
function Read-StateTable([string]$path) {
	$rows = [System.Collections.Generic.List[double[]]]::new()
	foreach ($line in [System.IO.File]::ReadLines($path)) {
		if ($line.Length -eq 0 -or -not [char]::IsDigit($line[0])) { continue }
		$fields = $line.Split(',')
		if ($fields.Count -ne 7) { throw "Malformed row in ${path}: $line" }
		$rows.Add([double[]]($fields | ForEach-Object { [double]::Parse($_, $culture) }))
	}
	return , $rows
}

function Get-HermitePosition($rows, [double]$julianDate) {
	$low = 0
	$high = $rows.Count - 1
	while ($high - $low -gt 1) {
		$mid = [int](($low + $high) / 2)
		if ($rows[$mid][0] -le $julianDate) { $low = $mid } else { $high = $mid }
	}
	$a = $rows[$low]
	$b = $rows[$high]
	$h = $b[0] - $a[0]
	$t = ($julianDate - $a[0]) / $h
	$h00 = 2 * $t * $t * $t - 3 * $t * $t + 1
	$h10 = $t * $t * $t - 2 * $t * $t + $t
	$h01 = -2 * $t * $t * $t + 3 * $t * $t
	$h11 = $t * $t * $t - $t * $t
	return @(1, 2, 3 | ForEach-Object { $h00 * $a[$_] + $h10 * $h * $a[$_ + 3] + $h01 * $b[$_] + $h11 * $h * $b[$_ + 3] })
}

$planetTables = @{}
foreach ($planet in @('mercury', 'venus', 'earth', 'mars', 'jupiter', 'saturn', 'uranus', 'neptune', 'pluto')) {
	$path = Join-Path $ProjectRoot "assets\trajectory\planets\$($planet)_heliocentric.csv"
	if (-not (Test-Path -LiteralPath $path)) {
		$failures.Add("Missing ephemeris table for $planet.")
		continue
	}
	$planetTables[$planet] = Read-StateTable $path
	if ($planetTables[$planet].Count -lt 600) {
		$failures.Add("$planet table has only $($planetTables[$planet].Count) rows.")
	}
}
$voyager = Read-StateTable (Join-Path $ProjectRoot 'assets\trajectory\voyager2_heliocentric.csv')
if ($voyager.Count -lt 10000) {
	$failures.Add("Voyager table has only $($voyager.Count) rows; the dense encounter windows are missing.")
}

# --- Flyby geometry against NASA's published closest approaches. ---
$encounters = @(
	@{ Id = 'jupiter'; RadiusKm = 69911.0; Km = 721670.0; Jd = 2444064.437 },
	@{ Id = 'saturn'; RadiusKm = 58232.0; Km = 161000.0; Jd = 2444842.642 },
	@{ Id = 'uranus'; RadiusKm = 25362.0; Km = 107000.0; Jd = 2446455.249 },
	@{ Id = 'neptune'; RadiusKm = 24622.0; Km = 29240.0; Jd = 2447763.664 }
)
foreach ($encounter in $encounters) {
	$table = $planetTables[$encounter.Id]
	$best = [double]::MaxValue
	$bestJd = 0.0
	foreach ($row in $voyager) {
		if ([Math]::Abs($row[0] - $encounter.Jd) -gt 0.5) { continue }
		$planet = Get-HermitePosition $table $row[0]
		$dx = $row[1] - $planet[0]
		$dy = $row[2] - $planet[1]
		$dz = $row[3] - $planet[2]
		$distance = [Math]::Sqrt($dx * $dx + $dy * $dy + $dz * $dz) * $kilometresPerAu
		if ($distance -lt $best) {
			$best = $distance
			$bestJd = $row[0]
		}
	}
	$errorFraction = [Math]::Abs($best - $encounter.Km) / $encounter.Km
	$timeErrorMinutes = [Math]::Abs($bestJd - $encounter.Jd) * 1440.0
	if ($errorFraction -gt 0.02) {
		$failures.Add(("{0} closest approach {1:N0} km differs from published {2:N0} km." -f $encounter.Id, $best, $encounter.Km))
	}
	if ($timeErrorMinutes -gt 5.0) {
		$failures.Add(("{0} closest approach is {1:N1} minutes from the published time." -f $encounter.Id, $timeErrorMinutes))
	}
	$clearance = 1.5 + [Math]::Sqrt($best / $encounter.RadiusKm)
	if ($encounter.Id -eq 'saturn' -and $clearance -le 2.334) {
		$failures.Add('Saturn flyby clearance is inside the F ring.')
	}
	Write-Host ("[LAYOUT] {0}: {1:N0} km ({2:N2} radii) at JD {3:F4}; display clearance {4:N2} radii" -f $encounter.Id, $best, ($best / $encounter.RadiusKm), $bestJd, $clearance)
}

# --- Source contracts. ---
$applicationSource = Get-Content -Raw (Join-Path $ProjectRoot 'src\core\Application.cpp')
foreach ($planet in @('saturn', 'uranus', 'jupiter', 'neptune')) {
	if ($applicationSource -notmatch "buildRingSystem\(`"$planet`"") {
		$failures.Add("$planet ring system is missing.")
	}
}
if ($applicationSource -match 'j2000TrueAnomaliesDegrees|kPlanetDaysPerSecond') {
	$failures.Add('Planets are animated by a private clock instead of the dated ephemeris.')
}

if ($failures.Count -gt 0) {
	$failures | ForEach-Object { Write-Error "[LAYOUT] $_" -ErrorAction Continue }
	exit 1
}

Write-Host '[LAYOUT] Size order, ring/moon clearances, planet spacing, Horizons tables and all four flyby geometries satisfy the regression contract.'
