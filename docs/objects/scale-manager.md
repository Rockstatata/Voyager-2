# ScaleManager: presentation scale

![Scientific overview from the H key](images/runtime/01_overview.jpg)

*Runtime capture, `H`: the whole compressed system at launch date. The planets are visible discs rather than sub-pixel points, and the order of orbits and sizes is real.*

A literal model cannot show a 13 m spacecraft, 25,000 km planets and a 119 AU heliosphere in one navigable view. `ScaleManager` (`src/scene/ScaleManager.*`) owns four explicit, monotonic mappings. The physical values in `CelestialBodyData` and in the Horizons tables are never modified. Only the rendered transforms use these mappings.

## 1. Body radii: power law around Earth

```text
renderRadius = 0.50 * (radiusKm / 6371)^0.60
```

| Body | Radius (km) | Render radius |
| --- | ---: | ---: |
| Jupiter | 69,911 | 2.105 |
| Saturn | 58,232 | 1.886 |
| Uranus | 25,362 | 1.145 |
| Neptune | 24,622 | 1.125 |
| Earth | 6,371 | 0.500 |
| Ganymede | 2,634 | 0.294 |
| Mercury | 2,440 | 0.281 |
| Moon | 1,737 | 0.229 |
| Miranda | 236 | 0.069 |

The exponent keeps every size comparison pointing the right way: Ganymede is larger than Titan, and Titan is larger than Mercury. It shrinks the 290:1 range between Jupiter and Miranda to 30:1, so moons and small planets stay visible beside giants. This replaces the earlier strictly linear scale, in which Earth was 0.014 units and effectively invisible, which is why the scene looked empty.

**The Sun is display-capped at 6.0 units.** The same law would give 8.4 units, crowding Mercury's 10.6-unit perihelion. It is still the largest body by a factor of 2.9.

## 2. Heliocentric distance: power law around Mercury

```text
renderDistance = 12 * (distanceAu / 0.387098)^0.55
```

That gives Mercury 12.0, Venus 16.9, Earth 20.2, Mars 25.5, Jupiter 50.1, Saturn 70.1, Uranus 102.8, Neptune 131.5, the termination shock (84 AU) 231 and the heliopause (119 AU) 280. Planets, Voyager, belts, heliosphere circles and the comet all use this one law, through `Trajectory::mapHeliocentricToRender` for dated positions ([mission-ephemeris.md](mission-ephemeris.md)). `distanceScaleAtAu` returns its local derivative `0.55 * renderDistance / distanceAu`, which is the magnification at any radius.

## 3. Moon orbits: parent-relative square root

```text
moonOrbit = parentRenderRadius * (1.6 + sqrt(semiMajorAxisKm / parentRadiusKm))
```

The mapping is monotonic, so every moon keeps its real order. The innermost moons land at about 3.9 parent radii (Tethys 3.85, Miranda 3.86, Io 4.06), outside Saturn's F ring at 2.33 and Uranus's epsilon ring at 2.02. Outer moons stay close enough to frame with their planet: Callisto is at 6.79 and Iapetus at 9.42 parent radii.

### Parent-scale compensation

A moon is a scene-graph child, and its world matrix is `planet.worldMatrix() * moon.localMatrix()`. The planet's matrix already contains the planet's scale, so a moon's local position and local scale are both divided by the parent's render radius:

```text
moon.localScale    = moonRenderRadius / parentRenderRadius
moon.orbitRadius   = moonOrbit / parentRenderRadius         (in parent radii)
```

Skipping this division makes every moon too large by the parent's scale and places it at the wrong distance. Rings use the same convention: their radii are authored in planet radii, so they need no correction.

## 4. Spacecraft: linear metres

```text
renderSize = metres * 0.006
```

All of Voyager's sourced dimensions share this one factor (`kSpacecraftUnitsPerMetre`), so its internal proportions are exact: the 3.7 m dish is 0.0222 units and the 13 m magnetometer boom is 0.078 units. The factor was raised from 0.002 so the craft reads clearly in chase shots and Inspect mode; the probe is still tiny beside Neptune (radius 1.125). The chase and Inspect cameras and logarithmic depth ([lighting.md](lighting.md)) make it inspectable down to a 5 cm lens.

## Honesty statement

The overview is an educational diagram. Directions from the Sun, dates, body order, orbital order and moon order are real. Absolute distances and the ratio of radius to distance are compressed. The HUD always reports physical values: AU, km/s and km from the Horizons data.

## Regression check

```powershell
.\scripts\verify_scene_layout.ps1
```

This script recomputes the mappings above. It checks size order, ring and moon clearance, planet spacing, the spacecraft scale against Neptune, the Horizons table format and the four flyby geometries.
