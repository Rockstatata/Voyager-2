# 2. Building geometry from scratch

[Guide index](README.md) · previous: [How a frame is drawn](01-opengl-pipeline.md) · next: [Transforms and cameras](03-transforms-and-cameras.md)

No mesh in this project is imported. Every vertex, normal, texture coordinate and triangle index is computed by a generator in `src/rendering/`. Each generator is a pure function: it takes a few numbers and returns `MeshData { vector<Vertex> vertices; vector<uint32_t> indices; }`, and it never calls OpenGL. `Mesh` (chapter 1) uploads the result.

This chapter derives every generator. The recipe is always the same:

1. **Place vertices** with a formula. This usually means looping over one or two parameters, such as angle and height.
2. **Give each vertex a normal**: a unit vector perpendicular to the surface at that point, pointing *outward*. Lighting depends entirely on it (chapter 5).
3. **Give each vertex a UV**: where in the texture image it samples, with (0,0) at the bottom-left of the image and (1,1) at the top-right.
4. **Stitch triangles** by listing vertex indices three at a time, **counter-clockwise as seen from outside** (chapter 1.5).

## 2.1 The grid trick (used by almost everything)

Most surfaces here are a *grid* of vertices: rows × columns. With `rowWidth` vertices per row, the vertex at (row, column) is at index `row * rowWidth + column`. Each grid cell (a quad) becomes two triangles:

```text
 topLeft ---- topRight          triangle 1: topLeft, topRight, bottomLeft
    |      /     |              triangle 2: topRight, bottomRight, bottomLeft
    |    /       |
 bottomLeft - bottomRight       (both counter-clockwise when seen from outside)
```

```cpp
const uint32_t topLeft     = row * rowWidth + column;
const uint32_t topRight    = topLeft + 1;
const uint32_t bottomLeft  = topLeft + rowWidth;
const uint32_t bottomRight = bottomLeft + 1;
```

## 2.2 The UV sphere: every planet and moon

`UvSphereGenerator::generate(latitudeSegments, longitudeSegments)`. The project calls `generate(32, 64)` **once** and shares that one mesh (`shared_ptr<Mesh>`) among all 26 bodies. A body's size comes from its scale transform, not from a different mesh.

### Vertices

A point on a unit sphere is described by two angles:

- **polar angle φ** from the north pole: 0 at the north pole, π at the south pole.
- **azimuth θ** around the Y axis: 0 to 2π.

```text
x = sin(φ) · cos(θ)
y = cos(φ)
z = sin(φ) · sin(θ)
```

The generator loops `latitude = 0..32` (33 rows) and `longitude = 0..64` (65 columns):

```cpp
const float polarAngle = kPi * latitude / latitudeSegments;          // φ
const float ringRadius = std::sin(polarAngle);                        // radius of this latitude circle
const float y          = std::cos(polarAngle);
...
const float azimuth = 2.0f * kPi * longitude / longitudeSegments;     // θ
vertex.position = glm::vec3(ringRadius * std::cos(azimuth), y, ringRadius * std::sin(azimuth));
vertex.normal   = glm::normalize(vertex.position);                    // on a unit sphere, normal == position
vertex.texCoord = glm::vec2(1.0f - longitudeFraction, 1.0f - latitudeFraction);
```

**Vertex count** = (32 + 1) × (64 + 1) = **2,145**.

**Why 65 columns, not 64?** Column 64 has the same *position* as column 0 (θ = 2π = 0) but a different UV: u = 0 instead of u = 1. If the last column reused vertex 0, the final strip of triangles would interpolate u from about 0.98 back to 0.0, squeezing the whole texture backwards into one thin strip: a visible seam. The duplicated column lets the texture wrap cleanly at u = 0/1.

**Why `1 - longitudeFraction`?** Increasing θ runs *westward* when seen from outside. Photographs of planets are stored with longitude increasing *eastward* to the right. Reversing u keeps continents the right way round. `validateSphere` checks this: it throws if increasing u does not follow the geographic-east tangent `cross(up, normal)`.

**Why `1 - latitudeFraction`?** Latitude row 0 is the north pole, and v = 1 is the *top* row of the image once `stb_image` flips it (chapter 4).

### Triangles

Each of the 32 × 64 cells is split into two triangles, but the top row and bottom row are special. At the north pole, all 65 vertices of row 0 sit at the same point (0,1,0), so the cell's "top" triangle would have zero area. Such triangles are skipped:

```cpp
if (latitude > 0)                       // not touching the north pole
    { topLeft, topRight, bottomLeft }
if (latitude + 1 < latitudeSegments)    // not touching the south pole
    { topRight, bottomRight, bottomLeft }
```

**Triangle count** = 2 × 64 × (32 − 1) = **3,968**, all non-degenerate. `validateSphere` rejects any triangle whose cross product is near zero.

Worked example with `generate(2, 4)`: 3 rows × 5 columns = 15 vertices. Row 1 is the equator. Two latitude bands give 4 + 4 = 8 triangles, a double pyramid. That is the smallest valid sphere.

Full reference: [uv-sphere.md](../objects/uv-sphere.md).

## 2.3 The box: spacecraft panels, blankets, instrument housings

`BoxGenerator::generate(width, height, depth)`, centred on the origin. `generate()` makes a unit cube that the part's scale stretches.

A cube has only 8 corners, but the generator makes **24 vertices**: 4 per face × 6 faces. Each corner is shared by three faces that point in three different directions, and a vertex can only have *one* normal. Sharing the corner would average the three normals and make the cube look like a blurry blob. Duplicating it gives each face its own flat normal, so the edges stay sharp.

```cpp
// +X face: normal (1,0,0), corners counter-clockwise seen from +X
{ { 1, 0, 0 }, {{ { x,-y,-z }, { x, y,-z }, { x, y, z }, { x,-y, z } }} },
...
uvs = { (0,0), (0,1), (1,1), (1,0) }            // every face shows the whole image
indices per face = { base, base+1, base+2,   base, base+2, base+3 }   // 2 triangles: a fan from corner 0
```

**24 vertices, 36 indices, 12 triangles.**

## 2.4 The cylinder, frustum and cone: booms, RTGs, cameras, thrusters, the bus

`CylinderGenerator::generate(radiusBottom, radiusTop, height, radialSegments, capBottom, capTop)`. The axis is +Y, centred on the origin. With different radii it is a *frustum* (the thruster nozzles and the X-band feed). With one radius 0 it is a cone. With `radialSegments = 10` it is a decagonal prism: that is exactly how Voyager's ten-sided bus is made.

### Side wall

Two rings of `radialSegments + 1` vertices (bottom at y = −h/2, top at y = +h/2). The seam column is duplicated for the same reason as the sphere.

```cpp
vertex.position = (radius · cosθ, y, radius · sinθ)
vertex.normal   = normalize(height · cosθ, −(radiusTop − radiusBottom), height · sinθ)
vertex.texCoord = (1 − fraction, ring)        // u around, v = 0 bottom / 1 top
```

**Where the normal comes from.** Two tangent vectors lie on the side surface:

- around the circle: T = (−sinθ, 0, cosθ);
- up the slanted side, from bottom rim to top rim: S = ((rTop − rBottom)·cosθ, height, (rTop − rBottom)·sinθ).

Their cross product S × T is perpendicular to both, which gives (height·cosθ, −Δr, height·sinθ). For a straight cylinder, Δr = 0 and the normal is horizontal. For a cone that narrows upward (Δr < 0), the normal tilts up, as it should.

Each segment is one quad: `{bottomLeft, bottomRight, topLeft}` and `{bottomRight, topRight, topLeft}`. If one radius is zero, the quads at the tip would be zero-area, so one triangle per segment is emitted instead.

### Caps

A *fan*: one centre vertex plus a ring. The cap has its own copies of the rim vertices, because a cap vertex needs the flat normal (0, ±1, 0), not the side normal. Cap UVs map the disc into the texture's inscribed circle: `(0.5 + 0.5cosθ, 0.5 + 0.5sinθ)`. The top cap winds `{centre, a, b}` and the bottom cap `{centre, b, a}`, so each is counter-clockwise from its own outside.

Counts for `generate(r, r, h, 16)` with both caps: sides 2 × 17 = 34 vertices and 32 triangles; each cap 1 + 17 = 18 vertices and 16 triangles. In total **70 vertices, 64 triangles**.

## 2.5 The parabolic dish: Voyager's 3.7 m high-gain antenna

`ParabolicDishGenerator::generate(radius, depth, thickness, radialSegments, radialRings)`. Voyager uses `(1.85 m, 0.38 m, 0.045 m, 64, 12)`, converted to render units.

A paraboloid is a parabola spun around its axis: **y = −depth + depth·(r/R)²**. The vertex is at the bottom (y = −depth, r = 0) and the rim is at y = 0 (r = R).

### Normal from the slope

For a surface y = f(r), the slope is dy/dr = 2·depth·r / R². At angle θ, the surface normal is proportional to (−slope·cosθ, 1, −slope·sinθ). This comes from the same "cross the two tangents" idea as the cylinder. In the code:

```cpp
const float slope = 2.0f * depth * r / (radius * radius);
glm::vec3 normal = glm::normalize(glm::vec3(-slope * cosTheta, 1.0f, -slope * sinTheta));
```

At the centre the slope is 0, so the normal points straight up the axis. Toward the rim it tilts inward, which is why a lit dish shows a bright ring where the tilt faces the Sun.

### Topology

- One **centre vertex** plus `radialRings` rings of `radialSegments + 1` vertices.
- The centre fan: `{center, b, a}` per segment. The rings: grid quads (2.1).
- UVs are "disc mapped": `(0.5 + 0.5·(r/R)·cosθ, 0.5 + 0.5·(r/R)·sinθ)`, the same as a cap.
- The generator builds this surface **twice**: the front (the concave side, normals up), and a back copy shifted down by `thickness` with the normals negated and the winding reversed.
- Only the two outer rings are joined by a strip of quads, which makes the rim edge. The shell is closed and has real thickness, so the ray tracer and the shadows see a solid dish.

## 2.6 The ring (annulus): Saturn, Uranus, Jupiter, Neptune rings

`RingGenerator::generate(innerRadius, outerRadius, radialSegments)` makes a flat washer in the XZ plane.

- For each of 2 faces (top normal +Y, bottom normal −Y): an inner ring and an outer ring of `segments + 1` vertices. `uv = (fraction around, 0 inner / 1 outer)`.
- The top face winds `{innerLeft, outerLeft, innerRight}` and `{innerRight, outerLeft, outerRight}`. The bottom face uses the reversed order, so it is counter-clockwise when seen from below.

Each planet gets one annulus per named **band** in `assets/data/ring_bands.csv`. Saturn has D, C, B, A and F bands, and the Cassini Division is a gap, with no geometry, between B (outer edge 1.951 radii) and A (inner edge 2.027 radii). See [rings.md](../objects/rings.md).

## 2.7 Line and point meshes

- **`CircleGenerator::generate(segments)`**: `segments` vertices on the unit circle and indices 0..n−1, drawn as `GL_LINE_LOOP`, which closes the last edge itself. Orbit guides use their own ellipse vertices built the same way (`EnvironmentBuilder`).
- **`StarfieldGenerator::generate(count, radius, seed)`**: uniform random points on a sphere, drawn as `GL_POINTS`. The key detail is uniformity. θ is uniform in [0, 2π), and **cos φ is uniform in [−1, 1]**, not φ itself. Picking φ uniformly would bunch stars at the poles, because latitude circles near the poles are small. A fixed seed (`std::mt19937`) makes the sky the same every run.

## 2.8 Assemblies: combining generator output

Complex parts are not new generators. They are generator output **transformed and appended** into one `MeshData`. `appendTransformed` in src/scene/VoyagerModelBuilder.cpp:

```cpp
const uint32_t firstVertex = destination.vertices.size();
const glm::dmat3 normalMatrix = glm::transpose(glm::inverse(glm::dmat3(transform)));
for each source vertex:
    position = transform * vec4(position, 1)
    normal   = normalize(normalMatrix * normal)      // see chapter 3.2 for why the inverse-transpose
for each source index:
    destination.indices.push_back(firstVertex + index)   // re-base the indices!
```

The re-basing (`firstVertex + index`) is the essential step. The second mesh's index 0 must point at *its own* first vertex, which is now stored after all the first mesh's vertices.

Built on top of this:

- **`appendRod(start, end, radius)`**: a thin cylinder from any point to any other. The generator's cylinder is along +Y and centred. `rotateYTo(end − start)` builds the quaternion that turns +Y onto the rod direction: the axis is `cross(Y, dir)` and the angle is `acos(dot(Y, dir))`, with special cases for parallel and anti-parallel directions. The rod is then translated to the midpoint.
- **`buildTriangularTruss(start, end, halfWidth, bays, rodRadius)`**: three long rails at 120° around the axis (offsets `sideA`, `−½sideA + (√3/2)sideB`, `−½sideA − (√3/2)sideB`). Each bay adds one diagonal per face, with alternating direction. This is how the 13 m magnetometer Astromast, the RTG boom and the science boom are made.
- **RTG fins**: a unit box with a hand-built matrix whose columns are the fin's axes, already scaled: `[boom direction × 0.50 m, radial × 0.10 m, tangent × 0.02 m, centre]`. Writing a matrix column by column like this is "change of basis" in its plainest form.
- **Bent dish ribs**: each rib follows the dish's back surface. It is six short rods whose end points lie on z = rimZ + depth·(1 − x²) + clearance, not one straight rod that would cut through the bowl.

The whole Voyager model is **11,652 triangles** across 18 inspectable components. See chapter 8.

## 2.9 Checking your geometry

- The UV sphere validates itself (counts, unit length, UV range, degenerate triangles, mirrored longitude) and throws `std::logic_error` if anything is wrong.
- In the running app, **F3 → Flat** shading makes every triangle visible, because each one is shaded with a single normal. An inside-out triangle shows up as a hole, since it is culled.
- The ray tracer (F9) intersects the *exact* triangles. Overlapping or intersecting parts become obvious there, which is how the dish-through-bus bug was found and fixed.
