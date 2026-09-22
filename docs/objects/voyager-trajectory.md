# Voyager 2 historical trajectory

## Source and offline asset

`assets/trajectory/voyager2_heliocentric.csv` contains 164 position samples requested from the official [NASA/JPL Horizons API](https://ssd-api.jpl.nasa.gov/doc/horizons.html):

- target `-32` — Voyager 2;
- center `500@10` — heliocentric, Sun center;
- reference plane `ECLIPTIC`, reference system `ICRF`;
- output units `AU-D`, position vector table;
- 1977-08-21 through 2030-01-01, primarily every 120 days, with exact rows for the four giant-planet encounter dates.

The API signature was checked as `NASA/JPL Horizons API` before the rows were captured. The CSV is committed so classroom/lab execution is deterministic and offline; the application makes no network request.

## Loading and coordinate mapping

`Trajectory::loadCsv` parses each row into `TrajectorySample { julianDate, heliocentricAu }`. Malformed rows are skipped with a `[TRAJECTORY]` diagnostic, and at least two valid samples are required.

Horizons ecliptic coordinates use X/Y for the ecliptic plane and Z for height. The scene uses X/Z for its orbital plane and Y for height, so each vector maps as:

```text
sceneVector = (horizonsX, horizonsZ, horizonsY)
direction = normalize(sceneVector)
renderRadius = ScaleManager::distanceToRenderUnits(length(sceneVector) * kmPerAU)
renderPosition = sunPosition + direction * renderRadius
```

This preserves the real heliocentric direction and applies the same documented `distanceAu^0.60` heliocentric compression used by planets and belts. It does not alter the checked-in source values.

## Line geometry

The trajectory mesh has one vertex and one sequential index per source sample. `PrimitiveMode::LineStrip` maps to `GL_LINE_STRIP`, so OpenGL connects sample `i` to `i+1` without incorrectly closing the final point back to launch. There are 164 vertices, 164 indices, 163 visible segments, and one draw call. The line uses a muted blue-grey flat material so it stays legible without overpowering Voyager.

## Historical controller

`Voyager2::setHistoricalPath` receives mapped positions and their Julian dates. Historical mode advances in Julian-date time, linearly interpolating between neighboring source rows until the final date, where it stops rather than looping back to launch. Its heading is updated from the current travel direction. `V` switches between this playback and inertial Manual mode.

## Accuracy boundary

The path is derived from real JPL vectors, not invented waypoints. However:

- 120-day sampling plus linear interpolation is visualization-level, not navigation-level;
- Earth, Mars, Jupiter, Saturn, Uranus, and Neptune use a date-anchored Kepler propagation: a Horizons sample supplies initial phase and each body's actual period/eccentricity supplies continuous motion;
- each exact encounter sample preserves its JPL approach direction but is displayed at four planet radii from the corresponding visual planet center, avoiding a false collision caused by display scaling.

## Verification

1. Startup prints `[TRAJECTORY] loaded 164 NASA/JPL Horizons samples` plus six 7-row planet tracks (Earth, Mars, and four giant planets).
2. A continuous muted line extends from the launch-era inner system into the outbound direction.
3. In Historical mode Voyager moves along that line and turns with it.
4. Press `V` to pilot manually, then `V` again to resume historical playback.
5. Press `T` to toggle the line; press `1`–`6` to verify all mission bookmarks and chase-camera jumps.
