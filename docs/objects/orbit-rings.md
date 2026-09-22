# Planet Orbit Guides

Each planet has one faint `SceneObject` guide named `<id>_orbit`. Guides are deliberately simple, complete ellipses: the sparse mission-era source points are used only to anchor the starting phase of historical motion and are not rendered as long crossing chords.

## Vertex and index construction

Every guide has 192 `Vertex` records, indexed `0..191`, and uses `GL_LINE_LOOP`; it has no triangles. For index `i`, `theta = 2*pi*i/192`. With display semi-major axis `a` and physical eccentricity `e`, the polar radius is `r = a(1-e^2)/(1+e*cos(theta))`; the vertex is `(r*cos(theta), 0, r*sin(theta))`. Normal `(0,1,0)` and UV are inert attributes required by the shared vertex format.

The guide is a scene root positioned at the Sun with identity scale. It therefore never inherits the Sun's visible radius.

## Motion relationship

Earth, Mars, Jupiter, Saturn, Uranus, and Neptune take their initial direction from the first checked-in JPL Horizons sample. During historical playback, the application advances mean anomaly from that date using each body's real orbital period, solves Kepler's equation by eight Newton iterations, then derives true anomaly and radius. This creates continuous curved motion rather than turning sharply at sparse data anchors. Mercury, Venus, and Pluto use the same Kepler ellipse model from their registered phase.

The Voyager path is hidden on startup. Press `T` only when the mission trace is wanted; hiding it keeps the ordinary solar-system overview readable.

## Moon paths

Moon guide lines are omitted because they would be sub-pixel clutter in an all-system view. Moons are actual children of their planet and update from parent-relative orbital transforms and their own orbital periods. Their display distance is power-compressed only for readability, preserving the order of every moon and its clearance from the planet/rings.

## Verification

- Press `H` for overview: only clean elliptical planet guides are present, with no intersecting mission-data chords.
- Press `1` through `5`: planets move continuously at each historical date and Voyager does not reset to launch after reaching the final date.
- Focus a moon using `Tab`: it remains orbiting its parent rather than the Sun.
