# Historical planet tracks

These small offline CSV tracks are NASA/JPL Horizons heliocentric vectors in the same ECLIPTIC/ICRF AU-D convention as `../voyager2_heliocentric.csv`.

They contain launch, Jupiter, Saturn, Uranus, Neptune, heliopause-era, and 2030 anchors. During Voyager's Historical mode the application interpolates these tracks by Julian Date and places Earth, Mars, and the four giant planets with the same ecliptic-to-scene mapping and distance compression as Voyager. This makes Mars date-correct throughout the mission and keeps the four encounter bookmarks spatially coherent; educational orbital animation remains available in Manual mode.

Targets: Earth `399`, Mars `499`, Jupiter `599`, Saturn `699`, Uranus `799`, Neptune `899`; center: `500@10` (Sun). The Neptune row at JD `2447763.5` and the matching Voyager row were requested for 1989-08-25, the Neptune encounter date.
