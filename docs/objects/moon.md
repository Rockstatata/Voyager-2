# Moon

![Runtime Focus view of Moon](images/bodies/moon.jpg)

*Annotated runtime capture: `Tab` Focus view of Moon at launch date 1977-08-21, Sun lighting on, labels and HUD visible (`Voyager-2.exe --capture-bodies <dir>`).*

## Overview

`Moon` is a `CelestialBody` (Moon) registered by `SolarSystem` as parent `"earth"`. It draws with the one shared 32x64 UV-sphere `Mesh` ([uv-sphere.md](uv-sphere.md)); this page covers only what is specific to Moon: scale, motion, physical data, material and verification.

## Geometry generation

None of its own. Moon contributes zero unique vertices: all 26 bodies share one sphere upload (2,145 vertices, 3,968 triangles, bible F9). The derivation of every vertex, UV, index and the duplicated seam column is in [uv-sphere.md](uv-sphere.md).

## Vertex attributes / triangle construction

Unchanged shared layout: `position` (location 0), `normal` (1), `texCoord` (2), counter-clockwise triangles — see [uv-sphere.md](uv-sphere.md) and [phase-2-rendering-pipeline.md](phase-2-rendering-pipeline.md).

## Transform

- World radius: `0.2293` = 0.50 x (1,737.4 / 6,371)^0.60; Earth's is `0.5000`.
- Local scale: `0.4586` = world radius / Earth radius, because the parent's world matrix multiplies it by Earth's scale again ([scale-manager.md](scale-manager.md), parent-scale compensation).
- Rotation: `angleAxis(6.68 deg, +Z)` tilt; spin applied to the mesh only.

## Motion

Local ellipse around Earth (`CelestialBody::setOrbit`, [orbital-motion.md](orbital-motion.md)) in Earth's tilted equatorial frame:

- semi-major axis `9.3676` parent radii = 1.6 + sqrt(384,400 / 6,371.0) (`ScaleManager::moonOrbitDistanceToRenderUnits`), eccentricity `0.0549`;
- angular rate `0.0920` rad/s = 2 pi x 0.4 / 27.322 days (moon visual clock: 0.4 days per second at 1x);
- starting true anomaly `0.0` deg (sibling 0 x golden angle 137.5 deg), giving local position `(8.8533, 0, 0.0000)`.

## Worked vertex example

Local equator vertex `(1, 0, 0)` -> scale `0.4586` -> tilt 6.68 deg about +Z -> `(0.4555, 0.0533, 0)` (spin 0) -> plus the body position. That sum is still in Earth's local frame; the parent world matrix (its tilt, scale and position) is applied next. Every matrix is double precision until `Renderer::submit` subtracts the camera position and narrows to float.

## Physical data

Source: NASA Planetary Fact Sheets (https://nssdc.gsfc.nasa.gov/planetary/factsheet/); positions of planets from NASA/JPL Horizons.

- Radius: 1,737.4 km
- Semi-major axis: 384,400 km
- Eccentricity: 0.0549
- Orbital period: 27.322 days
- Rotation period: 655.7 hours
- Axial tilt: 6.68 degrees

## Material mapping

- Texture file: `assets/textures/bodies/moon.jpg` (2048x1024, loaded via `Texture2D::loadFromFile` → `stb_image` decode → `glTexImage2D`)
- Source and credit: Solar System Scope free 2k texture pack, CC BY 4.0 (https://www.solarsystemscope.com/textures/)
- Shading: `Lit` (Sun point light, Lambert diffuse + Blinn-Phong specular strength 0, ambient 0.07) — [lighting.md](lighting.md). `K` toggles lighting off to show the raw texture.
- UVs come from the shared sphere generator; swapping the texture changes no vertex data.

## Limitations

- Radius is power-law compressed (size order preserved, absolute ratios not); see [scale-manager.md](scale-manager.md).
- Moon orbital positions run on a visual clock, not the dated ephemeris; real relative periods are preserved.
- Perfect sphere: no oblateness, no shadows cast onto rings or moons.

## Verification

- Startup log: `[SCENE] 26 bodies share 1 sphere mesh (2145 vertices, 3968 triangles), 26 real texture maps loaded`.
- Visual: `Tab` until the HUD shows `CAMERA FOCUS: MOON`; the camera flies in on the day side and the label reads `MOON`. Wheel zooms to the surface, drag orbits, `W` leaves into free flight.
