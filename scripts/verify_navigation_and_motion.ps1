param(
	[string]$ProjectRoot = (Split-Path -Parent $PSScriptRoot)
)

# Source-level regression contract for navigation, flight and time. Each
# check guards a behaviour that was once broken or explicitly requested.

$ErrorActionPreference = 'Stop'
function Read-Source([string]$relative) {
	return Get-Content -Raw (Join-Path $ProjectRoot $relative)
}

$application = Read-Source 'src\core\Application.cpp'
$mission = Read-Source 'src\scene\MissionController.cpp'
$cameraController = Read-Source 'src\core\CameraController.cpp'
$camera = Read-Source 'src\rendering\Camera.cpp'
$voyager = Read-Source 'src\scene\Voyager2.cpp'
$clock = Read-Source 'src\scene\SimulationClock.cpp'
$body = Read-Source 'src\scene\CelestialBody.cpp'
$renderer = Read-Source 'src\rendering\Renderer.cpp'
$fragment = Read-Source 'shaders/scene.frag'
$lightingModel = Read-Source 'shaders/lighting.glsl'
$ephemeris = Read-Source 'src\scene\MissionEphemeris.cpp'
$trajectory = Read-Source 'src\scene\Trajectory.cpp'
$missionHeader = Read-Source 'src\scene\MissionController.h'
$environment = Read-Source 'src\scene\EnvironmentBuilder.cpp'
$failures = [System.Collections.Generic.List[string]]::new()

function Require([bool]$condition, [string]$message) {
	if (-not $condition) { $failures.Add($message) }
}

# Time: one clock, no wrap-around, planets on the dated ephemeris.
Require ($clock -notmatch 'fmod') 'Historical time wraps back to launch.'
Require ($clock -match 'std::clamp\(m_julianDate, m_start, m_end\)') 'The simulation date is not clamped to the data range.'
Require ($mission -match 'placeBodies' -and $mission -match 'planetRenderPosition') 'Planets are not placed from the shared dated ephemeris.'
Require ($mission -match 'voyagerRenderPosition\(julianDate\)') 'Historical Voyager does not read the shared date.'
Require ($body -match 'const glm::dmat4 surface = surfaceMatrix\(\)' -and $body -match 'submit\(\*drawMesh, \*material\(\), surface\)') 'Planet spin is applied to the child frame and drags moons round.'
# Time never freezes: planets keep orbiting past the end of the NASA tables.
Require ($ephemeris -match 'setOrbitalExtrapolation\(true\)' -and $trajectory -match 'propagateKepler') 'Planets stop moving (or fly off in straight lines) after the ephemeris ends.'
Require ($mission -match 'kExtendedEndJulianDate' -and $missionHeader -match 'kExtendedEndJulianDate = 2634166\.5') 'The clock stops at the end of the NASA/JPL data instead of running on.'

# Camera autonomy.
Require ($camera -match 'scrollDelta' -and $camera -match 'm_speedMultiplier') 'Free flight has no wheel-controlled speed.'
Require ($cameraController -match 'nearestSurfaceDistance') 'Free-flight speed does not adapt to the nearest surface.'
Require ($camera -match 'kTransitionSeconds') 'Focus has no smooth fly-to transition.'
Require ($cameraController -match 'movementKey && !pilotingVoyager') 'Fly keys do not hand locked views to free flight.'
Require ($application -match 'group\("mission_path"\)\.setVisible\(false\)') 'The long Voyager trajectory is shown by default.'
Require ($application -match 'submitBackground') 'The starfield is not a camera-centred background.'

# Manual flight: full six degrees of freedom.
foreach ($key in @('GLFW_KEY_R', 'GLFW_KEY_F', 'GLFW_KEY_Q', 'GLFW_KEY_E', 'GLFW_KEY_X')) {
	Require ($voyager -match $key) "Manual flight is missing $key (pitch/roll/brake)."
}
Require ($voyager -match 'm_orientation \* turn') 'Manual rotations are not about the ship''s own axes.'

# Rendering: floating origin, log depth, lighting.
Require ($renderer -match 'relative\[3\] -= glm::dvec4\(m_origin, 0\.0\)') 'Model matrices are not camera-relative.'
Require ($fragment -match 'gl_FragDepth') 'Logarithmic depth is missing.'
# Performance contracts (docs/guide/11-performance.md).
Require ($renderer -match 'isVisible' -and $body -match 'renderer\.isVisible') 'Bodies are drawn even when outside the view frustum.'
Require ($body -match 's_lodLow' -and $application -match 'generate\(8, 16\)') 'Small bodies are not drawn with a coarser shared sphere.'
Require ($application -match 'renderShadowMap' -and $fragment -match 'shadowMapVisibility') 'Voyager self-shadows do not use the shadow map.'
Require ($fragment -notmatch 'sunVisibility\([^)]*, true\)') 'The raster pass traces Voyager''s BVH per pixel again (slow).'
Require ($environment -match 'UvSphereGenerator::generate\(3, 6\)') 'Belt rocks use a high-polygon sphere.'
foreach ($technique in @('SHADING_FLAT', 'SHADING_GOURAUD', 'SHADING_PHONG', 'SHADING_BLINN_PHONG', 'SHADING_TOON', 'LIGHT_DIRECTIONAL', 'LIGHT_POINT', 'LIGHT_SPOT')) {
	Require ($lightingModel -match $technique) "Lighting model is missing $technique."
}

if ($failures.Count -gt 0) {
	$failures | ForEach-Object { Write-Error "[NAVIGATION] $_" -ErrorAction Continue }
	exit 1
}

Write-Host '[NAVIGATION] Shared dated clock (running past the data), free-flight autonomy, 6-DOF piloting, floating origin, log depth, lighting and performance contracts satisfy the regression contract.'
