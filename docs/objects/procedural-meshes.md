# Procedural Mesh Construction Handbook

This is the exact geometry reference for every reusable mesh generator in the project. Object pages explain which generator and transform they use; this page explains how the actual vertices and indices are produced. Together they are sufficient to reconstruct the submitted geometry without an imported model.

## Shared vertex and index format

Every generated vertex is eight consecutive 32-bit floats (`32 bytes`):

| Attribute | GLSL location | Components | Byte offset | Meaning |
| --- | ---: | ---: | ---: | --- |
| `position` | 0 | 3 | 0 | local-space `(x,y,z)` |
| `normal` | 1 | 3 | 12 | local unit surface direction |
| `texCoord` | 2 | 2 | 24 | `(u,v)` texture coordinate |

`MeshData::vertices` holds those records and `MeshData::indices` holds unsigned 32-bit vertex indices. Triangle meshes submit each consecutive index triple to `GL_TRIANGLES`. Counter-clockwise order as viewed from outside is the project convention. Lines and points still use the same vertex structure, but OpenGL interprets their indices using `GL_LINE_LOOP`, `GL_LINE_STRIP`, or `GL_POINTS`.

## Unit box: `BoxGenerator`

For requested width `w`, height `h`, and depth `d`, define `x=w/2`, `y=h/2`, `z=d/2`. The box is centered on the local origin. Each face owns four vertices even though adjacent faces occupy the same corner positions; this is required because a hard edge needs two different face normals and may need two different UVs.

The six face corner lists, in emitted order, are:

```text
+X: ( x,-y,-z), ( x, y,-z), ( x, y, z), ( x,-y, z)  normal ( 1, 0, 0)
-X: (-x,-y, z), (-x, y, z), (-x, y,-z), (-x,-y,-z)  normal (-1, 0, 0)
+Y: (-x, y,-z), (-x, y, z), ( x, y, z), ( x, y,-z)  normal ( 0, 1, 0)
-Y: (-x,-y, z), (-x,-y,-z), ( x,-y,-z), ( x,-y, z)  normal ( 0,-1, 0)
+Z: ( x,-y, z), ( x, y, z), (-x, y, z), (-x,-y, z)  normal ( 0, 0, 1)
-Z: (-x,-y,-z), (-x, y,-z), ( x, y,-z), ( x,-y,-z)  normal ( 0, 0,-1)
```

Every face uses UVs `(0,0), (0,1), (1,1), (1,0)`. If `b` is that face's first vertex, its triangles are exactly `(b,b+1,b+2)` and `(b,b+2,b+3)`. Therefore one box always has 24 vertices, 36 indices, and 12 triangles.

## Cylinder, cone, and frustum: `CylinderGenerator`

The generator's axis is local `+Y`; the bottom and top planes are `y=-h/2` and `y=+h/2`. Let bottom/top radii be `rb` and `rt`, segment count `n`, seam-inclusive ring width `q=n+1`, and `f=i/n`, `theta=2*pi*f`.

For ring `k=0` (bottom) or `k=1` (top):

```text
y(k) = k == 0 ? -h/2 : +h/2
r(k) = k == 0 ? rb   : rt
position(k,i) = (r(k) cos(theta), y(k), r(k) sin(theta))
normal(k,i)   = normalize(h cos(theta), -(rt-rb), h sin(theta))
uv(k,i)       = (1-f, k)
index(k,i)    = k*q+i
```

Indices `i=0` and `i=n` have the same position but distinct `u=1` and `u=0` texture coordinates. For both radii non-zero, segment `i` uses:

```text
bottomLeft  = i              bottomRight = i+1
topLeft     = q+i            topRight    = q+i+1
triangles   = (bottomLeft,bottomRight,topLeft)
              (bottomRight,topRight,topLeft)
```

If one radius is zero, each segment emits one triangle instead of a zero-area quad. Each enabled cap has a separate center vertex and `n+1` rim vertices because its flat `(0,+/-1,0)` normal differs from the side normal. Cap UV is planar: `(0.5+0.5*cos(theta), 0.5+0.5*sin(theta))`. The top fan is `(center,a,b)`; the bottom reverses to `(center,b,a)`.

For the project's ordinary capped cylinder/frustum where `rb>0` and `rt>0`:

```text
vertices  = 2(n+1) + 2(n+2) = 4n+6
triangles = 2n side + n bottom + n top = 4n
indices   = 12n
```

For the capped-base cone used by the comet (`rt=0`), the unused top cap is omitted: `3n+4` vertices and `2n` triangles.

## Parabolic dish: `ParabolicDishGenerator`

The generated dish opens toward local `+Y`, has its rim at `y=0`, and its vertex at `y=-depth`. Let radius be `R`, depth `D`, thickness `T`, angular segments `n`, radial rings `m`, `t=j/m`, and `theta=2*pi*i/n`.

The front surface vertex is:

```text
r = R*t
y = -D + D*t^2
position = (r*cos(theta), y, r*sin(theta))
slope = 2*D*r/(R^2)
normal = normalize(-slope*cos(theta), 1, -slope*sin(theta))
uv = (0.5 + 0.5*t*cos(theta), 0.5 + 0.5*t*sin(theta))
```

The rear repeats the same positions at `y-T` and negates the normal. Each surface begins with one center vertex and then `m` rings of `n+1` vertices; the last angular vertex duplicates the seam position for continuous UVs.

For the front center fan, where `c` is the center and `a/b` are consecutive vertices of ring 1, the indices are `(c,b,a)`. The rear reverses them to `(c,a,b)`. Between adjacent rings the front uses `(innerLeft,innerRight,outerLeft)` and `(innerRight,outerRight,outerLeft)`; the rear reverses both. The outer front and rear rings are joined by two rim-wall triangles per segment.

With Voyager's `n=48`, `m=8`:

```text
vertices per face = 1 + 8*(48+1) = 393
total vertices    = 786
triangles/face    = 48 + 7*(2*48) = 720
rim triangles     = 2*48 = 96
total triangles   = 720+720+96 = 1,536
total indices     = 4,608
```

## Double-sided annulus: `RingGenerator`

Planetary bands lie in local `XZ` at `y=0`. For segment `i`, `f=i/n`, `theta=2*pi*f`, and radius `r` chosen as inner or outer:

```text
position = (r*cos(theta), 0, r*sin(theta))
normal   = (0,+1,0) for top or (0,-1,0) for bottom
uv       = (f, 0) for inner or (f, 1) for outer
```

Each face has separate inner and outer rings of `n+1` vertices. The top quad uses `(innerLeft,outerLeft,innerRight)` and `(innerRight,outerLeft,outerRight)`; the bottom reverses winding. Counts are `4(n+1)` vertices, `4n` triangles, and `12n` indices. Every current band uses `n=64`: 260 vertices and 256 triangles.

## Unit line circle: `CircleGenerator`

For `n` segments, vertex/index `i` is:

```text
theta = 2*pi*i/n
position = (cos(theta), 0, sin(theta))
normal = (0,1,0)       // unused by line rendering
uv = (i/n,0)           // unused
index = i
```

`GL_LINE_LOOP` supplies the final edge from `n-1` back to `0`; no duplicate seam vertex and no triangles are necessary. Orbit and heliosphere loops use `n=96`.

## UV sphere: `UvSphereGenerator`

The exact sphere derivation, pole exceptions, seam duplication, indices, normals, UV orientation, counts, and generator validation are documented in [uv-sphere.md](uv-sphere.md). The full body sphere is `32x64`: 2,145 vertices and 3,968 triangles. Belt rocks use `6x8`: 63 vertices and 80 triangles. Every star, planet, moon, comet nucleus, and instanced small body that appears spherical links back to that one construction.

## Star points: `StarfieldGenerator`

There are no triangles. For two deterministic uniform random samples `a,b` in `[0,1]`:

```text
theta  = 2*pi*a
cosPhi = 2*b-1
sinPhi = sqrt(1-cosPhi^2)
position = radius*(sinPhi*cos(theta), cosPhi, sinPhi*sin(theta))
index = point number
```

Uniform `cosPhi`, rather than uniform `phi`, avoids polar clustering. The submitted field contains 4,000 vertices/indices rendered as `GL_POINTS` with seed 1.

## Moving and merging generated parts

`SceneObject` parts normally keep their generator mesh unchanged and apply a model matrix during rendering. Composite meshes such as Voyager's trusses and RTGs instead bake transforms into one `MeshData`:

```text
newPosition = M * vec4(oldPosition,1)
normalMatrix = transpose(inverse(mat3(M)))
newNormal = normalize(normalMatrix * oldNormal)
newIndex = destinationVertexCountBeforeAppend + oldIndex
```

Using the inverse-transpose for normals is essential when a box is scaled differently on each axis. A rod between endpoints `A` and `B` starts as a `+Y` cylinder, has height `length(B-A)`, is rotated from `+Y` onto `normalize(B-A)`, and is translated to midpoint `(A+B)/2`.

## Which object uses which construction

| Construction | Rendered objects |
| --- | --- |
| 32×64 UV sphere | Sun, 8 planets, Pluto, 16 moons, comet nucleus |
| 6×8 UV sphere | asteroid belt, Kuiper belt, Oort-cloud instances |
| Annulus | every Jupiter/Saturn/Uranus/Neptune ring band |
| Unit line circle | eight orbit guides; three great circles per heliosphere boundary |
| Star points | background starfield |
| Box | Voyager panels, instruments, magnetometers, scan platform, RTG fins |
| Cylinder/frustum | Voyager bus, rods, antennas, cameras, RTGs, record, thrusters; comet tail |
| Parabolic dish | Voyager high-gain antenna |

This table is also the reason the project does not contain a separate handwritten triangle list for every planet: those objects intentionally share the exact same sphere vertices and differ only by transform/material. Their individual pages document those differences.
