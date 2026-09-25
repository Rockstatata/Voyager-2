# Planet ephemeris tables

NASA/JPL Horizons heliocentric state vectors for every planet and Pluto, in the same convention as `../voyager2_heliocentric.csv`: centre `500@10`, ICRF, ecliptic of J2000, columns `julian_date,x_au,y_au,z_au,vx_au_d,vy_au_d,vz_au_d`, 1977-08-21 to 2030-01-02. Targets are body centres, not system barycentres.

| File | Target | Step | Rows |
| --- | --- | --- | ---: |
| `mercury_heliocentric.csv` | 199 | 4 days | 4,782 |
| `venus_heliocentric.csv` | 299 | 5 days | 3,826 |
| `earth_heliocentric.csv` | 399 | 5 days | 3,826 |
| `mars_heliocentric.csv` | 499 | 5 days | 3,826 |
| `jupiter_heliocentric.csv` | 599 | 20 days | 957 |
| `saturn_heliocentric.csv` | 699 | 20 days | 957 |
| `uranus_heliocentric.csv` | 799 | 30 days | 638 |
| `neptune_heliocentric.csv` | 899 | 30 days | 638 |
| `pluto_heliocentric.csv` | 999 | 30 days | 638 |

The application cubic-Hermite interpolates these by the shared simulation Julian Date every frame, so planets and Voyager are always on the same date. Steps were chosen so the Hermite error is invisible: at worst about 5,000 km (Mercury near perihelion), under 0.001 render units; about 150 km for Neptune (its Triton wobble). Regenerate with `scripts/fetch_horizons.ps1`.
