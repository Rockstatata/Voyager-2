# Orbital Motion (Phase 5, elliptical-orbit placeholder)

## What it replaces

Through Phase 3, no body moved around anything — planets sat on a fixed line, moons sat at a fixed offset from their planet. `CelestialBody::setOrbit`/`updateOrbitPosition` (`src/scene/CelestialBody.h/.cpp`) adds real elliptical revolution, matching the bible's own staged progression (section 18): "static positions -> circular orbit approximation -> **orbital elements/ephemeris** -> synchronized historical positions." An initial pass implemented only the circular step; eccentricity was added the same session once the circular version was verified working, since the two share the same mechanism (a circle is just an ellipse with `eccentricity = 0`). Full ephemeris/synchronized-historical-position motion (matching a real date) is still future work (bible section 33), not attempted here — this is still uniform-angular-rate motion, not true Kepler timing (see "Known simplifications" below).

## The mechanism

```cpp
void setOrbit(semiMajorAxis, angularVelocity, initialAngleRadians, center = (0,0,0), eccentricity = 0);
// every frame, if this body has an orbit:
angle += angularVelocity * dt;                                    // dt already scaled by the simulation clock — see controls.md
r = semiMajorAxis * (1 - e^2) / (1 + e * cos(angle));              // polar conic-section equation, focus at `center`
position = center + r * (cos(angle), 0, sin(angle));
```

`angle` is treated as **true anomaly** — the angle measured from the focus (the Sun, or the planet a moon orbits), which is exactly where a real orbit's angle is conventionally measured from. The polar equation `r = a(1-e^2)/(1+e*cos(theta))` traces a mathematically exact ellipse with the orbited body sitting at one focus, for any `0 <= e < 1`. At `e = 0` it reduces to `r = a` (a constant radius — a circle), so this is a strict superset of the project's first circular-only version, not a separate code path.

One function, used identically for a planet orbiting the Sun and a moon orbiting a planet — `CelestialBody` doesn't know or care which. What differs is what the caller (`Application::buildScene`) passes in for `semiMajorAxis`, `angularVelocity`, `center`, and `eccentricity`.

## Planets: orbiting the Sun

Planets stay scene **roots** (not children of the Sun's `CelestialBody`) on purpose. A child's position composes through `parent.worldMatrix()`, which includes the parent's own scale — exactly the cascade bug `scale-manager.md` describes for moons. An orbit radius is an absolute world-space distance, unrelated to how large the Sun is currently drawn, so a planet becomes a root and receives the Sun's actual world position as its orbit `center` explicitly instead — same "moves together with its center" result, without the multiplication.

- `semiMajorAxis` = `ScaleManager::distanceToRenderUnits(semiMajorAxisKm)`.
- `eccentricity` = the body's real value (Mercury 0.2056 down to Venus's near-circular 0.0068; Pluto highest at 0.2488 — see `buildBodySpecs()` for the full table, sourced the same way as every other physical fact this project uses).
- `initialAngle` = `planetIndex * (360/8)` degrees — evenly spread at startup so the eight planets don't all start collinear (they did in Phase 3). Pluto (a ninth, optional body — bible section 3) isn't one of the 8 evenly-spaced slots; it falls back to the same angle as Mercury, which isn't a visible collision at Pluto's much larger orbit radius.
- `angularVelocity` = `2*pi*15 / orbitalPeriodDays` radians/second — "15" is `kPlanetDaysPerSecond`: 15 simulated days pass per one real second, before the simulation clock's own speed multiplier (`controls.md`) is applied on top. Every planet's angular speed stays proportional to its real one (Mercury really does orbit ~685x faster than Neptune), only the clock is sped up — the same compression technique already used for axial spin (`CelestialBody`'s `kVisualSpinSpeedup`), applied to revolution instead of rotation.

## Moons: orbiting their planet

Moons stay children of their planet (bible section 18: "Do not manually update the Moon's world coordinate from scratch if it can naturally be represented relative to Earth") — `center` stays the default `(0,0,0)`, i.e. the parent's own local origin, since the scene graph already supplies "moves together with the planet" for a child.

- `semiMajorAxis` = the same hand-tuned proximity formula Phase 3 used (`parentRenderRadius + moonRenderRadius + gap + siblingIndex*spacing`), **divided by `parentRenderRadius`** — the same cascade correction `scale-manager.md` describes. Deliberately *not* run through `ScaleManager::distanceToRenderUnits`: real moon-to-planet distances (384,400 km for the Moon) are a different order of magnitude than the planet distances that function is calibrated for, and rendering them "to real scale" would place outer moons many render-units from their already-compressed planet — worse for visibility, not better, which is the opposite of what this pass was fixing.
- `eccentricity` = the body's real value — mostly small (Tethys 0.0001, Triton ~0.00002) but not negligible for a few (the Moon 0.0549, Iapetus 0.0286, Titan 0.0288).
- `initialAngle` = `siblingIndex * 60` degrees.
- `angularVelocity` = `2*pi*0.4 / orbitalPeriodDays` radians/second — a **separate**, much smaller constant (`kMoonDaysPerSecond = 0.4`) than the planet one. Moon orbital periods (1.4-27 days) are 3-4 orders of magnitude smaller than planet periods (88-60,195 days); reusing the planet constant would either make moons blur past too fast to read as orbiting, or (using the moon constant for planets) make planets crawl. Both stay real-proportional within their own group.

## Known simplifications

- **Uniform angular rate, not true Kepler timing.** A real orbit sweeps equal areas in equal times (Kepler's second law) — faster at perihelion, slower at aphelion — which requires numerically solving Kepler's equation for eccentric/mean anomaly at every step. This project advances true anomaly at a constant rate instead, so the *shape* traced is a mathematically correct ellipse, but the *speed* along it isn't real-physical moment-to-moment.
- **Spin/orbit coupling for moons.** A moon's position is computed in its **local** space (relative to its planet), which is then composed through the planet's own `worldMatrix` — and that matrix includes the planet's axial-spin *rotation*, not just its position. Since both the planet's spin and the moon's own orbital motion rotate around the same local Y axis, the two angles add together in the final world result. The moon's own orbital motion still dominates in most cases (moon angular velocities are generally comparable to or faster than their planet's spin, at these compression constants), but this is a genuine remaining coupling, not fully decoupled. Fully separating "planet spin" from "what a child orbits around" needs a non-spinning intermediate pivot node between planet and moon — deferred; it was not required to fix the reported bug (moons being invisible/buried), which was a scale-cascade issue, not a rotation-coupling one.

## Verification

- `[SCENE]` log confirms build order (Sun, then 8 planets + Pluto, then all moons — parent-before-child, required by `SolarSystem::addBody`).
- Visual: planets visibly move along their `orbit-rings.md` preview circles over time; inner planets (Mercury, Venus) visibly complete revolutions faster than outer ones (Uranus, Neptune) within a short observation window; moons visibly circle their planet, offset from real time-zero (the tilt from `initialAngle`); Mercury and Pluto (the two highest-eccentricity bodies in the scene) visibly trace a non-circular path rather than a perfect circle.
- `controls.md`'s pause/speed keys visibly stop and rescale all of the above uniformly.
