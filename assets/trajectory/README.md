# Voyager trajectory data

`voyager2_heliocentric.csv` is an offline snapshot from the official [NASA/JPL Horizons API](https://ssd-api.jpl.nasa.gov/doc/horizons.html).

- Target: `-32` (Voyager 2)
- Center: `500@10` (Sun center)
- Ephemeris: vectors, position-only table
- Frame: ICRF; reference plane: ecliptic
- Units: AU and days
- Span: 1977-08-21 through 2030-01-01
- Step: primarily 120 days, with exact encounter-date rows for Jupiter, Saturn, Uranus, and Neptune
- Retrieved: 2026-09-21

The file contains 164 source rows only. Runtime mapping into educational render units is documented in [voyager-trajectory.md](../../docs/objects/voyager-trajectory.md). Keep this asset checked in so the submission remains deterministic and makes no network request in the lab.
