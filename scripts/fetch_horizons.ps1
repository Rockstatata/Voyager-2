# Regenerates the offline NASA/JPL Horizons ephemeris snapshots used by the
# application. The program itself never touches the network: these CSV files
# are checked in so the lab build is deterministic.
#
#   .\scripts\fetch_horizons.ps1
#
# Every file is a heliocentric (500@10) ECLIPTIC/ICRF state-vector table in
# AU and AU/day: julian_date,x_au,y_au,z_au,vx_au_d,vy_au_d,vz_au_d.
# Velocities let the runtime use cubic Hermite interpolation, so a 5-day
# Mercury table or a 1-minute Neptune-encounter table both stay smooth.

[CmdletBinding()]
param(
	[string]$ProjectRoot = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = 'Stop'
$api = 'https://ssd.jpl.nasa.gov/api/horizons.api'
$spanStart = '1977-08-21'
$spanStop = '2030-01-02'
$culture = [System.Globalization.CultureInfo]::InvariantCulture

function Get-HorizonsRows([string]$command, [string]$start, [string]$stop, [string]$step, [string]$center = '500@10')
{
	$query = [ordered]@{
		format = 'text'
		COMMAND = "'$command'"
		CENTER = "'$center'"
		EPHEM_TYPE = "'VECTORS'"
		START_TIME = "'$start'"
		STOP_TIME = "'$stop'"
		STEP_SIZE = "'$step'"
		OUT_UNITS = "'AU-D'"
		REF_PLANE = "'ECLIPTIC'"
		REF_SYSTEM = "'ICRF'"
		VEC_TABLE = "'2'"
		CSV_FORMAT = "'YES'"
		OBJ_DATA = "'NO'"
	}
	$pairs = $query.GetEnumerator() | ForEach-Object { "$($_.Key)=$([uri]::EscapeDataString($_.Value))" }
	$url = "$api`?$($pairs -join '&')"
	$text = (Invoke-WebRequest -UseBasicParsing -Uri $url -TimeoutSec 120).Content
	$inside = $false
	$rows = [System.Collections.Generic.List[string]]::new()
	foreach ($line in $text -split "`n")
	{
		if ($line.StartsWith('$$SOE')) { $inside = $true; continue }
		if ($line.StartsWith('$$EOE')) { break }
		if (-not $inside) { continue }
		$fields = $line.Split(',') | ForEach-Object { $_.Trim() }
		# JDTDB, calendar string, X, Y, Z, VX, VY, VZ
		$rows.Add(($fields[0], $fields[2], $fields[3], $fields[4], $fields[5], $fields[6], $fields[7]) -join ',')
	}
	if ($rows.Count -eq 0)
	{
		throw "Horizons returned no rows for $command ($center) $start..$stop ($step)"
	}
	Write-Host "[TRAJECTORY] $command (centre $center) $start..$stop @ $step -> $($rows.Count) rows"
	return $rows
}

function Write-StateTable([string]$path, [string[]]$comments, [object[]]$rows)
{
	# Merge overlapping windows: one row per Julian Date, ascending.
	$unique = @{}
	foreach ($row in $rows)
	{
		$unique[$row.Split(',')[0]] = $row
	}
	$sorted = $unique.Keys | Sort-Object { [double]::Parse($_, $culture) } | ForEach-Object { $unique[$_] }
	$lines = @($comments | ForEach-Object { "# $_" }) + 'julian_date,x_au,y_au,z_au,vx_au_d,vy_au_d,vz_au_d' + $sorted
	New-Item -ItemType Directory -Force (Split-Path -Parent $path) | Out-Null
	[System.IO.File]::WriteAllLines($path, $lines)
	Write-Host "[TRAJECTORY] wrote $($sorted.Count) rows -> $path"
}

# Near a planet, Voyager is written as planet (heliocentric) + Voyager
# (planet-centred). Horizons serves heliocentric Voyager from the spacecraft
# reconstruction but heliocentric planets from the current planetary
# ephemeris; at Neptune those disagree by ~13,600 km, while the planet-centred
# vector is the true flyby geometry (29,300 km at closest approach).
function Get-EncounterRows([string]$planet, [string]$start, [string]$stop, [string]$step)
{
	$relative = @(Get-HorizonsRows '-32' $start $stop $step "500@$planet")
	$planetRows = @(Get-HorizonsRows $planet $start $stop $step)
	if ($relative.Count -ne $planetRows.Count)
	{
		throw "Row count mismatch for $planet $start..$stop"
	}
	$rows = [System.Collections.Generic.List[string]]::new()
	for ($i = 0; $i -lt $relative.Count; $i++)
	{
		$r = $relative[$i].Split(',')
		$p = $planetRows[$i].Split(',')
		if ($r[0] -ne $p[0])
		{
			throw "Epoch mismatch for $planet at row $i"
		}
		$values = @($p[0])
		for ($k = 1; $k -le 6; $k++)
		{
			$sum = [double]::Parse($p[$k], $culture) + [double]::Parse($r[$k], $culture)
			$values += $sum.ToString('E15', $culture)
		}
		$rows.Add($values -join ',')
	}
	return $rows
}

function ConvertTo-JulianDate([datetime]$date)
{
	return $date.ToOADate() + 2415018.5
}

$retrieved = (Get-Date).ToString('yyyy-MM-dd')

# --- Planets and Pluto: body centres, not system barycentres. ---
$planets = [ordered]@{
	mercury = @{ id = '199'; step = '4d' }
	venus = @{ id = '299'; step = '5d' }
	earth = @{ id = '399'; step = '5d' }
	mars = @{ id = '499'; step = '5d' }
	jupiter = @{ id = '599'; step = '20d' }
	saturn = @{ id = '699'; step = '20d' }
	uranus = @{ id = '799'; step = '30d' }
	neptune = @{ id = '899'; step = '30d' }
	pluto = @{ id = '999'; step = '30d' }
}
foreach ($name in $planets.Keys)
{
	$entry = $planets[$name]
	$rows = Get-HorizonsRows $entry.id $spanStart $spanStop $entry.step
	Write-StateTable (Join-Path $ProjectRoot "assets\trajectory\planets\$($name)_heliocentric.csv") @(
		"NASA/JPL Horizons target $($entry.id) ($name), center 500@10, ECLIPTIC/ICRF, AU and AU/day",
		"State vectors $spanStart..$spanStop every $($entry.step); retrieved $retrieved by scripts/fetch_horizons.ps1"
	) $rows
}

# --- Voyager 2: a 5-day cruise table, replaced within +/-40 days of each
# giant-planet encounter by composed planet + planet-centred rows at 6 h,
# 10 min (+/-3 days) and 1 min (+/-6 h of the published closest approach). ---
$encounters = @(
	@{ id = '599'; date = [datetime]'1979-07-09'; closest = [datetime]'1979-07-09 22:29' },
	@{ id = '699'; date = [datetime]'1981-08-26'; closest = [datetime]'1981-08-26 03:24' },
	@{ id = '799'; date = [datetime]'1986-01-24'; closest = [datetime]'1986-01-24 17:59' },
	@{ id = '899'; date = [datetime]'1989-08-25'; closest = [datetime]'1989-08-25 03:56' }
)

$encounterSpans = foreach ($encounter in $encounters)
{
	@{
		start = ConvertTo-JulianDate $encounter.date.AddDays(-40)
		stop = ConvertTo-JulianDate $encounter.date.AddDays(40)
	}
}
$voyagerRows = [System.Collections.Generic.List[string]]::new()
foreach ($row in (Get-HorizonsRows '-32' $spanStart $spanStop '5d'))
{
	$jd = [double]::Parse($row.Split(',')[0], $culture)
	$insideEncounter = $false
	foreach ($span in $encounterSpans)
	{
		if ($jd -ge $span.start -and $jd -le $span.stop) { $insideEncounter = $true }
	}
	if (-not $insideEncounter)
	{
		$voyagerRows.Add($row)
	}
}

foreach ($encounter in $encounters)
{
	$date = $encounter.date
	$closest = $encounter.closest
	$windows = @(
		@($date.AddDays(-40).ToString('yyyy-MM-dd'), $date.AddDays(40).ToString('yyyy-MM-dd'), '6h'),
		@($date.AddDays(-3).ToString('yyyy-MM-dd'), $date.AddDays(3).ToString('yyyy-MM-dd'), '10m'),
		@($closest.AddHours(-6).ToString('yyyy-MM-dd HH:mm'), $closest.AddHours(6).ToString('yyyy-MM-dd HH:mm'), '1m')
	)
	foreach ($window in $windows)
	{
		foreach ($row in (Get-EncounterRows $encounter.id $window[0] $window[1] $window[2]))
		{
			$voyagerRows.Add($row)
		}
	}
}

Write-StateTable (Join-Path $ProjectRoot 'assets\trajectory\voyager2_heliocentric.csv') @(
	'NASA/JPL Horizons target -32 (Voyager 2), heliocentric (500@10), ECLIPTIC/ICRF, AU and AU/day',
	"Cruise every 5d $spanStart..$spanStop; within +/-40d of each giant-planet encounter rows are planet + planet-centred Voyager (6h, 10m within +/-3d, 1m within +/-6h of closest approach)",
	"Retrieved $retrieved by scripts/fetch_horizons.ps1"
) $voyagerRows.ToArray()
