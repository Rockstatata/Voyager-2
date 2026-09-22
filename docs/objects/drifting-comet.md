# Drifting Comet

Its sphere and cone use the exact shared formulas in [uv-sphere.md](uv-sphere.md) and the [Procedural Mesh Construction Handbook](procedural-meshes.md).

## Overview

`drifting_comet`, a group of two parts (nucleus + tail) that slowly orbits far beyond the Oort cloud. Replaces the earlier Andromeda placeholder — that flat disc read poorly on screen (a single flat-shaded circle, no depth cue without lighting) and was cut; this reuses the same "single distant object to anchor the far background" role with a shape that reads clearly even completely unlit.

## Geometry generation

Two meshes, both already used elsewhere in this project — no new generator:

- **Nucleus**: the shared 32x64 UV sphere (`m_sphereMesh`, [uv-sphere.md](uv-sphere.md)) — same mesh every planet/moon draws.
- **Tail**: `CylinderGenerator::generate(0.06, 0.0, 3.0, 10)` ([voyager-2.md](voyager-2.md) covers this generator in full) — a tapered cone, wide at the nucleus end, a point at the far end. `radiusTop = 0` is what makes it taper to a point rather than staying a uniform-width cylinder.

## Vertex attributes / triangle construction

Standard `Vertex{position, normal, uv}` for both parts — see [uv-sphere.md](uv-sphere.md) and [voyager-2.md](voyager-2.md).

## Transform

The group itself is a `CelestialBody`, used here purely as a moving container rather than for its physical data (bible section 22's orbit mechanism isn't specific to real celestial bodies) — its own `Mesh`/`Material` are left null, so `SceneObject::render` skips drawing the group itself and only its two children draw. Its `transform().scale` is left at identity (`1.0`) specifically so the nucleus and tail can use plain absolute local sizes with no cascade correction — that correction (`scale-manager.md`) is only needed when the *parent's* scale isn't 1.

- **Orbit** (`orbital-motion.md`'s mechanism, reused): radius 50 (beyond the Oort cloud's outer edge at 40), angular velocity 0.008 rad/s (~13 minutes per full revolution — "slowly drifting", not orbiting like a planet), initial angle 270 degrees (points it toward -Z from the Sun, matching the default camera's starting look direction), center = the Sun's world position.
- **Nucleus**: local position `(0,0,0)` (at the group's own origin), scale `0.15`.
- **Tail**: recomputed every frame in `Application::update` (not set once at construction) so it always points directly away from the Sun as the comet moves along its orbit — a real comet's tail does the same, since it's driven by solar wind/radiation pressure, not by the comet's direction of travel. Each frame:
  1. `awayFromSun = normalize(comet.position - sun.position)`.
  2. Build a quaternion rotating local +Y (the cone generator's default axis) onto `awayFromSun`: `axis = cross((0,1,0), awayFromSun)`, `angle = acos(dot((0,1,0), awayFromSun))`, `rotation = angleAxis(angle, normalize(axis))` — falling back to identity or a 180-degree flip when `axis` is degenerate (i.e. `awayFromSun` is already parallel or anti-parallel to +Y).
  3. Position the tail so its wide end still sits at the nucleus: `position = rotation * (0, halfLength, 0)` — the same "offset by half-length along the now-rotated axis" idea used for the fixed-direction version this replaced, just recomputed with a dynamic rotation instead of a constant one.

## Material mapping

Flat, bright pale cyan/white on both parts — no texture. "Shiny" here means bright flat color, not a lighting effect (this project has no lighting yet); the tail's taper is what actually reads as a comet shape at a glance, the color is secondary.

## Limitations

- Perfectly circular orbit (`eccentricity` defaults to 0 and isn't set for this object), not a real comet's extremely eccentric one — unlike every real body in the scene, which now uses real eccentricity (`orbital-motion.md`); this one could trivially be given a high eccentricity too, just wasn't since its orbit radius/speed were already hand-picked for framing, not sourced from a real comet's data.
- One comet, not a population.
- Tail length/width don't grow near the Sun and shrink far away, the way a real comet's tail does — constant size regardless of distance.

## Verification

- `[SCENE] environment built: ... 1 drifting comet with tail` startup log line.
- Visual: a small bright point with a tapered streak trailing it, visible near the default camera's starting view, position changing slowly (not static) across multiple observations; the tail's direction visibly changes as the comet moves along its orbit, always pointing away from the Sun rather than staying fixed relative to the nucleus.
