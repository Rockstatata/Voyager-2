# Titan

![Runtime Focus view of Titan](images/bodies/titan.jpg)

*Annotated runtime capture: `Tab` Focus view of Titan at launch date 1977-08-21, Sun lighting on, labels and HUD visible (`Voyager-2.exe --capture-bodies <dir>`).*

## Overview

`Titan` is a `CelestialBody` (Moon) registered by `SolarSystem` as parent `"saturn"`. It draws with the one shared 32x64 UV-sphere `Mesh` ([uv-sphere.md](uv-sphere.md)); this page covers only what is specific to Titan: scale, motion, physical data, material and verification.

## Geometry generation

None of its own. Titan contributes zero unique vertices: all 26 bodies share one sphere upload (2,145 vertices, 3,968 triangles, bible F9). The derivation of every vertex, UV, index and the duplicated seam column is in [uv-sphere.md](uv-sphere.md).

## Vertex attributes / triangle construction

Unchanged shared layout: `position` (location 0), `normal` (1), `texCoord` (2), counter-clockwise triangles — see [uv-sphere.md](uv-sphere.md) and [phase-2-rendering-pipeline.md](phase-2-rendering-pipeline.md).

## Transform

- World radius: `0.2903` = 0.50 x (2,574.7 / 6,371)^0.60; Saturn's is `1.8860`.
- Local scale: `0.1539` = world radius / Saturn radius, because the parent's world matrix multiplies it by Saturn's scale again ([scale-manager.md](scale-manager.md), parent-scale compensation).
- Rotation: `angleAxis(0.3 deg, +Z)` tilt; spin applied to the mesh only.

## Motion

Local ellipse around Saturn (`CelestialBody::setOrbit`, [orbital-motion.md](orbital-motion.md)) in Saturn's tilted equatorial frame:

- semi-major axis `6.1807` parent radii = 1.6 + sqrt(1,221,870 / 58,232.0) (`ScaleManager::moonOrbitDistanceToRenderUnits`), eccentricity `0.0288`;
- angular rate `0.1576` rad/s = 2 pi x 0.4 / 15.945 days (moon visual clock: 0.4 days per second at 1x);
- starting true anomaly `52.5` deg (sibling 3 x golden angle 137.5 deg), giving local position `(3.6927, 0, 4.8165)`.

## Worked vertex example

Local equator vertex `(1, 0, 0)` -> scale `0.1539` -> tilt 0.3 deg about +Z -> `(0.1539, 0.0008, 0)` (spin 0) -> plus the body position. That sum is still in Saturn's local frame; the parent world matrix (its tilt, scale and position) is applied next. Every matrix is double precision until `Renderer::submit` subtracts the camera position and narrows to float.

## Physical data

Source: NASA Planetary Fact Sheets (https://nssdc.gsfc.nasa.gov/planetary/factsheet/); positions of planets from NASA/JPL Horizons.

- Radius: 2,574.7 km
- Semi-major axis: 1,221,870 km
- Eccentricity: 0.0288
- Orbital period: 15.945 days
- Rotation period: 382.7 hours
- Axial tilt: 0.3 degrees

## Material mapping

- Texture file: `assets/textures/bodies/titan.jpg` (720x360, loaded via `Texture2D::loadFromFile` → `stb_image` decode → `glTexImage2D`)
- Source and credit: NASA 3D Resources (github.com/nasa/NASA-3D-Resources), public domain ("free and without copyright" per the repo README)
- Shading: `Lit` (Sun point light, Lambert diffuse + Blinn-Phong specular strength 0, ambient 0.07) — [lighting.md](lighting.md). `K` toggles lighting off to show the raw texture.
- UVs come from the shared sphere generator; swapping the texture changes no vertex data.

## Limitations

- Radius is power-law compressed (size order preserved, absolute ratios not); see [scale-manager.md](scale-manager.md).
- Moon orbital positions run on a visual clock, not the dated ephemeris; real relative periods are preserved.
- Perfect sphere: no oblateness, no shadows cast onto rings or moons.

## Verification

- Startup log: `[SCENE] 26 bodies share 1 sphere mesh (2145 vertices, 3968 triangles), 26 real texture maps loaded`.
- Visual: `Tab` until the HUD shows `CAMERA FOCUS: TITAN`; the camera flies in on the day side and the label reads `TITAN`. Wheel zooms to the surface, drag orbits, `W` leaves into free flight.
