# Scientific ratios and display scale

A literal one-scale model cannot show a 13 m spacecraft, kilometre-scale moons, and a 119 AU heliosphere in one useful OpenGL view. The final scene therefore uses two explicit mappings and does not claim that one screen is a literal photograph of the entire solar system.

## Exact celestial size ratios

Every Sun, planet, moon, and local moon-orbit measurement uses one linear factor:

```text
celestialUnitsPerKm = 1.5 / 696,340
renderRadius = physicalRadiusKm * celestialUnitsPerKm
moonWorldOrbit = parentRenderRadius * max((moonSemiMajorAxisKm / parentRadiusKm)^0.70, 3.0)
```

This makes the Sun radius `1.5` while preserving the exact source ratios: Jupiter `0.15059`, Saturn `0.12544`, Uranus `0.05464`, Neptune `0.05304`, Earth `0.01372`, and the Moon `0.00374`. Ring radii are already expressed in parent-radius units, so their ratios remain exact. Moon orbit *display distances* use a documented 0.70 power compression: this preserves distance order and safe ring clearance while preventing physically correct but unreadably spread-out moon systems in the heliocentric overview.

## Heliocentric overview spacing

Planet and Voyager distances from the Sun use a separate monotonic compression:

```text
distanceAu = distanceKm / 149,597,870.7
renderDistance = 3.0 * pow(distanceAu / 0.387098, 0.60)
```

This gives approximately Mercury `3.00`, Venus `4.36`, Earth `5.31`, Mars `6.82`, Jupiter `14.28`, Saturn `20.49`, Uranus `31.18`, and Neptune `40.77`. The exponent preserves physical order while keeping large, readable gaps between the outer planets. It is a documented overview compression, not a false claim of literal interplanetary scale.

Asteroid, Kuiper, heliosphere, Oort-cloud, comet, planet, and Voyager-trajectory positions all pass through the same heliocentric mapping. The Oort cloud uses 2,000-5,000 AU and therefore remains outside the normal planetary overview instead of obscuring it.

## Voyager

Voyager uses `0.0001` render unit per metre. Its 3.7 m dish is `0.00037` units and its 13 m boom is `0.0013` units, much smaller than Neptune's `0.05304` radius but still inspectable with the third-person camera. All internal spacecraft proportions remain linear.

## Camera

`Home` frames the compressed heliocentric overview. Bodies are intentionally small there because their mutual radius ratios are real. `Tab` focuses individual bodies. ThirdPerson uses a tighter clipping range for Voyager without changing object geometry.

## Regression check

```powershell
.\scripts\verify_scene_layout.ps1
```

The check verifies Sun/Jupiter/Earth size ratios, Saturn-ring/Tethys clearance, outer-planet spacing, Voyager/Neptune scale, J2000 planet phases, and all four historical encounter rows.

`verify_navigation_and_motion.ps1` additionally checks that no looping historical playback, sparse crossing planet paths, uncompressed moon display distance, or missing mouse-orbit camera behavior is reintroduced.
