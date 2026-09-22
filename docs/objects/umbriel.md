# Umbriel

## Overview

`Umbriel` is a `CelestialBody` (Moon) in the solar-system registry (`SolarSystem`), parent `"uranus"`. It draws using the single shared UV-sphere `Mesh` uploaded once at startup — see [uv-sphere.md](uv-sphere.md) for how that mesh's vertices and triangles are built. This document covers only what's specific to `Umbriel`: its transform, its real physical data, and its texture.

## Geometry generation

Not object-specific. `Umbriel` contributes zero unique vertices or triangles — it reuses the one `Mesh` shared by all 26 bodies in the scene (bible failure mode F9: never upload a mesh per body). Full derivation of that shared sphere (stacks/slices grid, pole handling, seam duplication, index winding) is in [uv-sphere.md](uv-sphere.md).

## Vertex attributes / triangle construction

Identical to every other body in the scene — see [uv-sphere.md](uv-sphere.md) and [phase-2-rendering-pipeline.md](phase-2-rendering-pipeline.md). `Umbriel` does not alter the `Vertex{position, normal, uv}` layout or the index buffer.

## Transform

- Class: `CelestialBody`, registry id `"umbriel"`, parent `"uranus"`
- Local scale: `0.2846` (uniform, all axes) — real radius 584.7 km compressed the same way, THEN DIVIDED by uranus's own render radius — required because this is a scene-graph child and uranus's `worldMatrix` will multiply this local scale by uranus's own scale again; skipping that division is the exact bug [scale-manager.md](scale-manager.md) documents (a moon rendering ~12x too close and ~8x too small, mostly buried inside its planet).
- Local position: `(-1.4074, 0, 2.4378)` — initial orbit position (moves every frame — see Orbit above)
- Rotation: `tilt * spin`, both quaternions. `tilt = angleAxis(radians(0), Z)` is constant. `spin = angleAxis(spinAngle, Y)` accumulates every frame in `CelestialBody::update(dt)` at `2*pi / (rotationPeriodHours * 3600) * kVisualSpinSpeedup` rad/s, where `kVisualSpinSpeedup = 3600` compresses time (1 simulated hour per real second) so the true rotation period stays proportional and relatively correct against every other body, just fast enough to see. `dt` here is already scaled by the simulation clock (`controls.md`) before this multiplication.

## Orbit

Elliptical orbit (`CelestialBody::setOrbit`, see [orbital-motion.md](orbital-motion.md)): semi-major axis `2.8095` render units, eccentricity `0.0039`, angular velocity `0.6065` rad/s, initial true anomaly `120` degrees, orbit center `(0.0, 0.0, 0.0)` (local (relative to its parent) coordinates). At start, `r = a(1-e^2)/(1+e*cos(angle0))` = `2.8149`, giving position `(-1.4074, 0, 2.4378)`. This point moves every frame — it is the position *at startup*, not a fixed value.

## Worked vertex example

At the position above, with `spinAngle = 0` (so `rotation = tilt` only), two vertices from the shared unit-sphere data show what the transform chain actually does to a stored coordinate:

- Local equator vertex `(1, 0, 0)` -> scale by `0.2846` -> `(0.2846, 0, 0)` -> rotate `0` degrees about +Z -> `(0.2846, 0, 0)` -> translate by local position -> world `(-1.1228, 0, 2.4378)`
- Local "north pole" vertex `(0, 1, 0)` -> scale -> `(0, 0.2846, 0)` -> rotate -> `(0, 0.2846, 0)` -> translate -> world `(-1.4074, 0.2846, 2.4378)`

(World here is local to the parent uranus, not the final on-screen position — the engine multiplies by the parent's world matrix too.) This is why a body's stored `position` is its center, not any one vertex — every vertex is offset from it by the same scale/rotate/translate chain, computed once per frame in `Transform::localMatrix()` and narrowed to `float` only at GPU upload (bible section 12).

## Physical data

Source: NASA Planetary Fact Sheets (https://nssdc.gsfc.nasa.gov/planetary/factsheet/), standard reference values, not flight-grade ephemeris.

- Radius: 584.7 km
- Semi-major axis: 266,000 km
- Eccentricity: 0.0039
- Orbital period: 4.144 days
- Rotation period: 99.5 hours
- Axial tilt: 0 degrees

## Material mapping

- Texture file: `assets/textures/bodies/umbriel.jpg` (1440x720, loaded via `Texture2D::loadFromFile` → `stb_image` decode → `glTexImage2D`)
- Source and credit: NASA 3D Resources (github.com/nasa/NASA-3D-Resources), public domain ("free and without copyright" per the repo README)
- UV coordinates come entirely from the shared sphere generator (`u = theta/2*pi`, `v = phi/pi`), computed independently of which texture file is bound — swapping `Umbriel`'s texture changes zero vertex data, which is the proof that texturing and geometry are decoupled pipeline stages (see [phase-2-rendering-pipeline.md](phase-2-rendering-pipeline.md)).

## Limitations

- Uniform angular rate, not true Kepler equal-area timing (see [orbital-motion.md](orbital-motion.md), "Known simplifications").
- Local orbit position is composed through `uranus`'s own `worldMatrix` (which includes uranus's axial-spin rotation, not just its position) — so this moon's effective angular position is (parent spin angle + this moon's own orbit angle), not purely its own orbit angle. See [orbital-motion.md](orbital-motion.md), "Known simplifications" — a genuine remaining coupling, not fully decoupled, but the moon's own orbital motion still dominates.
- No bump/normal map, atmosphere, or ring shading; diffuse color only.

## Verification

- Startup `[SCENE]` log line reports total body count and confirms all bodies share one mesh.
- Startup `[ASSET]` log line confirms `assets/textures/bodies/umbriel.jpg` loaded and its pixel resolution.
- Visual: `Umbriel` renders as a lit, textured sphere, visibly moving along its orbit over time; toggling `setVisible(false)` on it hides only this body.
