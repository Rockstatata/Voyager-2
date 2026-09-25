# Triton

![Runtime Focus view of Triton](images/bodies/triton.jpg)

*Annotated runtime capture: `Tab` Focus view of Triton at launch date 1977-08-21, Sun lighting on, labels and HUD visible (`Voyager-2.exe --capture-bodies <dir>`).*

## Overview

`Triton` is a `CelestialBody` (Moon) registered by `SolarSystem` as parent `"neptune"`. It draws with the one shared 32x64 UV-sphere `Mesh` ([uv-sphere.md](uv-sphere.md)); this page covers only what is specific to Triton: scale, motion, physical data, material and verification.

## Geometry generation

None of its own. Triton contributes zero unique vertices: all 26 bodies share one sphere upload (2,145 vertices, 3,968 triangles, bible F9). The derivation of every vertex, UV, index and the duplicated seam column is in [uv-sphere.md](uv-sphere.md).

## Vertex attributes / triangle construction

Unchanged shared layout: `position` (location 0), `normal` (1), `texCoord` (2), counter-clockwise triangles — see [uv-sphere.md](uv-sphere.md) and [phase-2-rendering-pipeline.md](phase-2-rendering-pipeline.md).

## Transform

- World radius: `0.1974` = 0.50 x (1,353.4 / 6,371)^0.60; Neptune's is `1.1252`.
- Local scale: `0.1754` = world radius / Neptune radius, because the parent's world matrix multiplies it by Neptune's scale again ([scale-manager.md](scale-manager.md), parent-scale compensation).
- Rotation: `angleAxis(157.0 deg, +Z)` tilt; spin applied to the mesh only.

## Motion

Local ellipse around Neptune (`CelestialBody::setOrbit`, [orbital-motion.md](orbital-motion.md)) in Neptune's tilted equatorial frame:

- semi-major axis `5.3960` parent radii = 1.6 + sqrt(354,800 / 24,622.0) (`ScaleManager::moonOrbitDistanceToRenderUnits`), eccentricity `2e-05`;
- angular rate `-0.4276` rad/s = 2 pi x 0.4 / -5.877 days (moon visual clock: 0.4 days per second at 1x) — negative: Triton orbits retrograde;
- starting true anomaly `0.0` deg (sibling 0 x golden angle 137.5 deg), giving local position `(5.3959, 0, 0.0000)`.

## Worked vertex example

Local equator vertex `(1, 0, 0)` -> scale `0.1754` -> tilt 157.0 deg about +Z -> `(-0.1615, 0.0685, 0)` (spin 0) -> plus the body position. That sum is still in Neptune's local frame; the parent world matrix (its tilt, scale and position) is applied next. Every matrix is double precision until `Renderer::submit` subtracts the camera position and narrows to float.

## Physical data

Source: NASA Planetary Fact Sheets (https://nssdc.gsfc.nasa.gov/planetary/factsheet/); positions of planets from NASA/JPL Horizons.

- Radius: 1,353.4 km
- Semi-major axis: 354,800 km
- Eccentricity: 2e-05
- Orbital period: 5.877 days (retrograde)
- Rotation period: 141.0 hours
- Axial tilt: 157.0 degrees

## Material mapping

- Texture file: `assets/textures/bodies/triton.jpg` (1440x720, loaded via `Texture2D::loadFromFile` → `stb_image` decode → `glTexImage2D`)
- Source and credit: NASA 3D Resources (github.com/nasa/NASA-3D-Resources), public domain ("free and without copyright" per the repo README)
- Shading: `Lit`, `ice` preset: specular strength 0.22, power 36, ambient 0.07, a derived normal map (relief 1.6). Lit by the Sun point light (plus the optional headlamp and fill) in whichever technique `F3` selects (default Blinn-Phong) — [lighting.md](lighting.md). `K` toggles lighting off to show the raw texture; `F8` toggles the maps.
- Shadows: every fragment traces a soft shadow ray to the Sun through all body spheres and ring bands, so this body both casts and receives eclipse shadows (`F4`: off, hard, soft). In the ray-traced view (`F9`) it is an exact analytic sphere with mirror reflectivity 0.06 — [ray-tracing.md](ray-tracing.md).
- UVs come from the shared sphere generator; swapping the texture changes no vertex data.

## Limitations

- Radius is power-law compressed (size order preserved, absolute ratios not); see [scale-manager.md](scale-manager.md).
- Moon orbital positions run on a visual clock, not the dated ephemeris; real relative periods are preserved.
- The NASA map leaves the hemisphere Voyager 2 never imaged black; the dark region in the capture is that data gap, not a lighting fault.
- Perfect sphere: no oblateness. Shadows come only from the Sun; the headlamp and fill cast none.

## Verification

- Startup log: `[SCENE] 26 bodies share 1 sphere mesh (2145 vertices, 3968 triangles), 26 real texture maps loaded`.
- Visual: `Tab` until the HUD shows `CAMERA FOCUS: TRITON`; the camera flies in on the day side and the label reads `TRITON`. Wheel zooms to the surface, drag orbits, `W` leaves into free flight.
