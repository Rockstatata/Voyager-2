# Venus

## Overview

`Venus` is a `CelestialBody` (Planet) in the solar-system registry (`SolarSystem`), parent none (root body). It draws using the single shared UV-sphere `Mesh` uploaded once at startup — see [uv-sphere.md](uv-sphere.md) for how that mesh's vertices and triangles are built. This document covers only what's specific to `Venus`: its transform, its real physical data, and its texture.

## Geometry generation

Not object-specific. `Venus` contributes zero unique vertices or triangles — it reuses the one `Mesh` shared by all 26 bodies in the scene (bible failure mode F9: never upload a mesh per body). Full derivation of that shared sphere (stacks/slices grid, pole handling, seam duplication, index winding) is in [uv-sphere.md](uv-sphere.md).

## Vertex attributes / triangle construction

Identical to every other body in the scene — see [uv-sphere.md](uv-sphere.md) and [phase-2-rendering-pipeline.md](phase-2-rendering-pipeline.md). `Venus` does not alter the `Vertex{position, normal, uv}` layout or the index buffer.

## Transform

- Class: `CelestialBody`, registry id `"venus"`, parent none (root body)
- Local scale: `0.118` (uniform, all axes) — real radius 6,051.8 km compressed by cube root against Earth's radius (`ScaleManager::radiusToRenderUnits`), floored at a minimum of 0.03 render units so small bodies stay visible — see [scale-manager.md](scale-manager.md).
- Local position: `(2.3192, 0, -27.6808)` — initial orbit position (moves every frame — see Orbit above)
- Rotation: `tilt * spin`, both quaternions. `tilt = angleAxis(radians(177.4), Z)` is constant. `spin = angleAxis(spinAngle, Y)` accumulates every frame in `CelestialBody::update(dt)` at `2*pi / (rotationPeriodHours * 3600) * kVisualSpinSpeedup` rad/s, where `kVisualSpinSpeedup = 3600` compresses time (1 simulated hour per real second) so the true rotation period stays proportional and relatively correct against every other body, just fast enough to see. `dt` here is already scaled by the simulation clock (`controls.md`) before this multiplication.

## Orbit

Elliptical orbit (`CelestialBody::setOrbit`, see [orbital-motion.md](orbital-motion.md)): semi-major axis `3.2957` render units, eccentricity `0.0068`, angular velocity `0.4194` rad/s, initial true anomaly `45` degrees, orbit center `(0.0, 0.0, -30.0)` (world (it's a scene root) coordinates). At start, `r = a(1-e^2)/(1+e*cos(angle0))` = `3.2798`, giving position `(2.3192, 0, -27.6808)`. This point moves every frame — it is the position *at startup*, not a fixed value.

## Worked vertex example

At the position above, with `spinAngle = 0` (so `rotation = tilt` only), two vertices from the shared unit-sphere data show what the transform chain actually does to a stored coordinate:

- Local equator vertex `(1, 0, 0)` -> scale by `0.118` -> `(0.118, 0, 0)` -> rotate `177.4` degrees about +Z -> `(-0.1178, 0.0054, 0)` -> translate by local position -> world `(2.2013, 0.0054, -27.6808)`
- Local "north pole" vertex `(0, 1, 0)` -> scale -> `(0, 0.118, 0)` -> rotate -> `(-0.0054, -0.1178, 0)` -> translate -> world `(2.3138, -0.1178, -27.6808)`

(World here is already the final position, since this body has no parent.) This is why a body's stored `position` is its center, not any one vertex — every vertex is offset from it by the same scale/rotate/translate chain, computed once per frame in `Transform::localMatrix()` and narrowed to `float` only at GPU upload (bible section 12).

## Physical data

Source: NASA Planetary Fact Sheets (https://nssdc.gsfc.nasa.gov/planetary/factsheet/), standard reference values, not flight-grade ephemeris.

- Radius: 6,051.8 km
- Semi-major axis: 108,208,000 km
- Eccentricity: 0.0068
- Orbital period: 224.701 days
- Rotation period: 5,832.5 hours (negative = retrograde spin)
- Axial tilt: 177.4 degrees

## Material mapping

- Texture file: `assets/textures/bodies/venus.jpg` (2048x1024, loaded via `Texture2D::loadFromFile` → `stb_image` decode → `glTexImage2D`)
- Source and credit: Solar System Scope free 2k texture pack, CC BY 4.0 (https://www.solarsystemscope.com/textures/)
- UV coordinates come entirely from the shared sphere generator (`u = theta/2*pi`, `v = phi/pi`), computed independently of which texture file is bound — swapping `Venus`'s texture changes zero vertex data, which is the proof that texturing and geometry are decoupled pipeline stages (see [phase-2-rendering-pipeline.md](phase-2-rendering-pipeline.md)).

## Limitations

- Uniform angular rate, not true Kepler equal-area timing (see [orbital-motion.md](orbital-motion.md), "Known simplifications").
- No bump/normal map, atmosphere, or ring shading; diffuse color only.

## Verification

- Startup `[SCENE]` log line reports total body count and confirms all bodies share one mesh.
- Startup `[ASSET]` log line confirms `assets/textures/bodies/venus.jpg` loaded and its pixel resolution.
- Visual: `Venus` renders as a lit, textured sphere, visibly moving along its orbit over time; toggling `setVisible(false)` on it hides only this body.
