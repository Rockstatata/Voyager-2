# Planet orbit guides

![Overview with the nine orbit guides](images/runtime/01_overview.jpg)

*Runtime capture, `H`: each planet sits on its own guide. The guides are 3D: Pluto's is visibly inclined, and Mercury's perihelion is off-centre.*

Each planet and Pluto has one faint `SceneObject` guide named `<id>_orbit`, which `O` toggles. A guide is the line its planet actually rides on, not a decorative circle.

## Construction

`MissionEphemeris::orbitGuide(planet, epoch, 360)` derives classical two-body elements from the planet's Horizons state vector at the epoch 1987-07-15 (JD 2447000.5, mid-mission), with `mu = 2.9591220828559115e-4 AU^3/day^2`:

```text
h = r x v                                   angular momentum (orbit normal)
e = (v x h) / mu - r / |r|                  eccentricity vector (points to perihelion)
a = 1 / (2 / |r| - |v|^2 / mu)              semi-major axis (vis-viva)
P = e / |e|,  Q = normalize(h) x P,  b = a sqrt(1 - |e|^2)
point(E) = P a (cos E - |e|) + Q b sin E    for E = 2 pi i / 360
```

Each 3D ellipse point is then mapped with the same `Trajectory::mapHeliocentricToRender` law used for the planet ([scale-manager.md](scale-manager.md)). Inclination, node and perihelion direction are therefore all real, and so is the compressed shape.

## Vertices and indices

Each guide has 360 `Vertex` records stored relative to the Sun (`point - sunPosition`) with indices `0..359`, drawn as `GL_LINE_LOOP`. OpenGL closes the last edge, and there are no triangles. The object is a scene root translated to the Sun with identity scale. Normal `(0,1,0)` and UV `(i/359, 0)` only satisfy the shared vertex format. The material is `Unlit`, a muted blue, or violet for Pluto.

## Accuracy

The ephemeris positions include planetary perturbations, but a single osculating ellipse does not. Between 1977 and 2030 the real positions drift from the 1987 osculating ellipse by a small fraction of a display radius. Each planet therefore stays visibly on its guide for the whole mission.

## Voyager path

The gold Voyager trajectory ([voyager-trajectory.md](voyager-trajectory.md)) is hidden at startup and toggled with `T`, which keeps the overview uncluttered.

## Moon paths

Moon guides are omitted, since at overview distance they would be sub-pixel clutter. Moons follow parent-relative ellipses ([orbital-motion.md](orbital-motion.md)).

## Verification

1. Press `H`. There are nine clean guides, with Pluto's crossing inside Neptune's and tilted out of the plane.
2. Press `=` several times. Each planet slides along its own guide and never leaves it.
3. Press `O` and all guides hide. Press it again and they return.
