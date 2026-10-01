# 11. Performance: measuring and optimising

[Guide index](README.md) · previous: [How to change things](10-how-to-change-things.md)

This chapter records how the project was diagnosed and made fast. Every change below was made because a measurement pointed at it, and every number comes from the built-in benchmark. Re-run it after any change you make.

## 11.1 Measure first: `--benchmark`

```powershell
x64\Release\Voyager-2.exe --benchmark bench.txt
```

`Benchmark` (src/core/Benchmark.cpp) turns **vsync off** (otherwise every frame waits for the monitor and reads 16.7 ms). It then visits eight fixed views. Each view settles for 2.5 s and is measured for 3 s:

| Column | Meaning |
| --- | --- |
| frame ms / fps | Whole-frame time on the CPU clock: update, draw submission and buffer swap |
| GPU ms | Time the GPU spent on `render()`, from an OpenGL `GL_TIME_ELAPSED` timer query. Queries are read one frame late so they never stall the pipeline. |
| draws | `glDraw*` calls the Renderer issued (`RenderStats`) |
| triangles | Triangles submitted that frame |

If GPU ms is close to frame ms, the GPU is the bottleneck. If frame ms is much larger, the CPU is.

## 11.2 What the measurements showed

Baseline (before optimisation), Radeon RX 590, 1440×900 window, Release build:

| View | frame ms | GPU ms | draws | triangles |
| --- | ---: | ---: | ---: | ---: |
| overview | 1.17 | 0.96 | 149 | 818,404 |
| launch_chase | 2.52 | 2.19 | 149 | 818,404 |
| jupiter_focus | 1.37 | 1.10 | 149 | 818,404 |
| saturn_focus | 1.50 | 1.23 | 149 | 818,404 |
| voyager_inspect | 4.19 | 3.93 | 149 | 818,404 |
| voyager_dish_closeup | **10.81** | **9.99** | 149 | 818,404 |
| saturn_raytraced | 2.57 | 2.10 | 24 | 687,968 |
| voyager_raytraced | **13.88** | **12.71** | 24 | 687,968 |

Diagnosis, one experiment at a time (shaders load at start-up, so each could be switched off without rebuilding):

1. **Voyager close-ups were 10× slower than anything else.** With the per-pixel shadow ray through Voyager's BVH switched off, the dish close-up fell from 9.99 ms to 0.90 ms. **About 90% of the frame was spent tracing shadow rays through 11,652 triangles for every Voyager pixel.**
2. **818,404 triangles every frame, even when nothing was close.** 680,000 of them were the 8,500 belt rocks, each a 6×8 sphere (80 triangles), although a rock is a few pixels at most. Every one of the 26 bodies also used the full 3,968-triangle sphere, even when it was a dot.
3. **Geometry used as texture.** As the teacher pointed out, parts of Voyager were modelled triangle by triangle where a photograph would do: louvre strips as extra boxes, 6-sided capped cylinders for 2 cm truss rods, a 64×12 dish, and ribs made of 72 rods. Voyager was 11,652 triangles.
4. **Ray-traced view.** Each Voyager pixel spent about 4.4 ms on primary rays, 1.8 ms on the shadow ray and 3.5 ms on the reflection bounce.
5. **Shadow rays on planets** ran `asin`/`acos` for every one of the 26 spheres at every lit pixel, including pixels facing away from the Sun, whose shadow cannot matter.
6. **CPU:** each draw call looked up ~13 uniform locations by building and hashing name strings, and the trace scene uploaded ~200 array elements one call at a time.

## 11.3 What was changed

| # | Optimisation | Where | Why it works |
| --- | --- | --- | --- |
| 1 | **Shadow mapping** for Voyager's self-shadows instead of per-pixel BVH rays | `ShadowMap`, `shaders/shadow.*`, `scene.frag` | One depth-only pass of Voyager from the Sun (5,828 triangles into a 2048² depth texture), then 9 filtered texture lookups per pixel. Ray-per-pixel cost depended on triangle count; shadow-map cost does not. |
| 2 | **Texture instead of geometry** | `VoyagerModelBuilder` | The louvred bays show NASA's louvre photograph instead of extra boxes. Rods are 4-sided open tubes (their ends are buried in joints). The dish is 48×6 instead of 64×12, and ribs use 4 rods each. Voyager: **11,652 → 5,828 triangles**, 82 → 76 parts, and close-ups look the same. |
| 3 | **Low-poly belt rocks** | `EnvironmentBuilder` | 3×6 sphere (24 triangles, faceted like a real asteroid) instead of 6×8 (80): **680k → 204k triangles**. |
| 4 | **Level of detail (LOD)** for bodies | `CelestialBody::render`, `Application::buildScene` | Three shared spheres: 32×64 (3,968 triangles), 16×32 (960) and 8×16 (224). A body uses the coarser ones when its apparent radius (radius ÷ distance) is below 0.05 (≈40 px) or 0.012 (≈10 px). |
| 5 | **Frustum culling** | `Renderer::isVisible`, `CelestialBody`, `Voyager2` | Six planes are extracted from projection × view each frame. A bounding sphere wholly outside any plane is not drawn. Voyager's 76 parts are skipped together when off screen or under half a pixel. |
| 6 | **Shadow-ray early-outs** | `scene.frag`, `raytrace.glsl` | No shadow work for pixels the Sun does not light. A cheap distance test rejects occluders before any trigonometry. |
| 7 | **Ordered BVH traversal** | `raytrace_mesh.glsl` | Both children are tested and the nearer is visited first, so the closest hit found shrinks the search and the farther box is often skipped. |
| 8 | **Reflection rays skip the mesh** | `raytrace.frag` | Bounced rays see planets and rings, not Voyager reflected in itself, so most of the bounce cost is gone. |
| 9 | **Cached uniform locations, batched arrays** | `Renderer::cacheUniformLocations`, `LightingUniforms` | Per-draw locations are looked up once. Arrays of basic types have consecutive locations, so each trace array is one `glUniform*v` call. |
| 10 | **SAH tree build** | `TriangleBvh::buildNode` | Split where the surface-area heuristic predicts the fewest tests, instead of at the median. Boxes hug real clusters of parts. |

## 11.4 Result

Same views, same machine, median of three runs (the GPU times repeat to within 0.01 ms):

| View | frame ms (before → after) | GPU ms (before → after) | draws | triangles |
| --- | ---: | ---: | ---: | ---: |
| overview | 1.17 → **0.60** | 0.96 → 0.36 | 149 → 67 | 818k → 236k |
| launch_chase | 2.52 → **0.69** | 2.19 → 0.54 | 149 → 118 | 818k → 237k |
| jupiter_focus | 1.37 → **0.58** | 1.10 → 0.43 | 149 → 43 | 818k → 232k |
| saturn_focus | 1.50 → **0.61** | 1.23 → 0.46 | 149 → 45 | 818k → 233k |
| voyager_inspect | 4.19 → **0.86** | 3.93 → 0.51 | 149 → 117 | 818k → 233k |
| voyager_dish_closeup | 10.81 → **0.97** | 9.99 → 0.81 | 149 → 118 | 818k → 237k |
| saturn_raytraced | 2.57 → **1.78** | 2.10 → 1.46 | 24 | 688k → 212k |
| voyager_raytraced | 13.88 → **5.53** | 12.71 → 4.78 | 24 | 688k → 212k |
| **mean** | **4.75 → 1.45** (3.3× faster) | | | |

In the normal (raster) view, every frame is now under 1 ms; the worst close-up is 11× faster. The ray-traced view (F9) is an optional, deliberately expensive mode. Its worst case is 2.5× faster, and the SAH tree build (row 10 of the table in 11.3) alone took it from 6.3 to 4.8 ms GPU. On a much weaker GPU (laptop integrated graphics can be 10–20× slower) the old raster close-ups would have run under 10 fps; they now stay comfortably real-time.

One experiment that failed is also worth knowing: raising the BVH traversal stack from 32 to 40 entries made *every* ray-traced view about 30% slower. A bigger local array in a shader uses more GPU registers, so fewer pixels run in parallel. The stack stays at 32, which covers the tree's depth of 24.

## 11.5 The techniques, explained

### Shadow mapping

A classic raster shadow technique, and now what Voyager uses for its own shadows:

1. **Light pass.** Render the shadow casters from the light's point of view into a depth texture. The Sun is so far away that its rays are parallel across a 20 m spacecraft, so the light camera is **orthographic**, fitted to Voyager's bounding sphere:

```cpp
lightView       = lookAt(centre + toSun * 2r, centre, up);   // camera-relative (floating origin)
lightProjection = ortho(-r, r, -r, r, 0, 4r);
shadowMatrix    = translate(0.5) * scale(0.5) * lightProjection * lightView;   // to texture space 0..1
```

2. **Camera pass.** For each spacecraft pixel at position p, `s = shadowMatrix * p` gives its texel in the map (s.xy) and its depth from the Sun (s.z). If the map stores a nearer depth at s.xy, something lies between p and the Sun, so p is in shadow.

```glsl
uniform sampler2DShadow shadowMap;     // GL_COMPARE_REF_TO_TEXTURE: texture() returns the comparison
float lit = texture(shadowMap, vec3(s.xy, s.z));   // GL_LINEAR -> 2x2 comparisons averaged by hardware
```

Soft mode averages a 3×3 ring of such lookups (**percentage-closer filtering**, PCF). Hard mode uses one.

**Shadow acne and bias.** The stored depth and the surface's own depth are nearly equal, and rounding makes surfaces shadow themselves in stripes. `glPolygonOffset(2, 4)` pushes the stored depths back during the light pass. The slope term makes the offset larger on surfaces at grazing angles to the Sun, where acne is worst.

**Why it is faster:** the per-pixel ray walked a tree of 11,652 triangles. The shadow map costs one cheap depth-only draw of the craft per frame plus 9 texture lookups per pixel, whatever the triangle count. (The ray-traced view, F9, still traces the BVH, because there the goal is exact ray tracing.)

### Level of detail

Rendering 3,968 triangles for a moon that covers 6 pixels wastes almost all of them; several triangles fall inside one pixel. LOD picks the mesh by **apparent size**:

```cpp
const double apparentSize = radius / distance(centre, camera);     // ≈ tan(angular radius)
if (apparentSize < 0.012)      drawMesh = sphere8x16;    // 224 triangles
else if (apparentSize < 0.05)  drawMesh = sphere16x32;   // 960 triangles
else                           drawMesh = sphere32x64;   // 3,968 triangles
```

All three spheres are uploaded **once** and shared by every body. They are made by the same `UvSphereGenerator`, so UVs and normals line up and textures look the same at each level.

### Frustum culling

The view frustum is the pyramid of space the camera can see. From the combined matrix `clip = projection × view`, each plane is a row sum (the Gribb–Hartmann method). For example, left = row3 + row0 and right = row3 − row0. A sphere is invisible if, for any plane, `dot(normal, centre) + offset < −radius`. That is six dot products to skip a whole draw call.

### The surface-area heuristic (SAH)

A ray hits a box with probability proportional to the box's surface area. Splitting a node's triangles into a left set L and a right set R therefore costs, on average,

```text
cost = area(box L) × count(L) + area(box R) × count(R)
```

`TriangleBvh::buildNode` sorts the centroids into 12 bins along each axis, scores all 11 bin boundaries per axis (two sweeps give every left and right box), and takes the cheapest. A median split cuts the triangle list in half regardless of where the parts are. SAH cuts through the empty space between the dish, the bus and the booms, so a ray that misses the dish never opens its boxes. The tree is deeper (24 levels, 6,723 nodes), but rays visit far fewer nodes.

### Texture versus geometry

A triangle is worth spending when it changes the **silhouette** or the **shading** at the size the viewer sees it. Surface detail such as louvre slats, foil wrinkles or crater relief is cheaper and better as a **texture** (colour) plus a **normal map** (lighting detail; chapter 4.5). Voyager's louvres, foil and blankets are now NASA photographs on simple boxes. Thin struts keep just enough sides to read as round.

## 11.6 Rules of thumb for future changes

- Measure with `--benchmark` before and after. Optimise the biggest number, not the most visible code.
- Per-pixel loops (rays, shadow tests, `for` over all spheres) cost *pixels × iterations*: keep them out of the common path or exit early.
- Small or distant things need few triangles. Use the LOD spheres; never upload a mesh per object.
- Prefer a texture region (chapter 4.4) to modelling surface detail.
- Upload uniforms once per frame where possible, and arrays in one call.
- `scripts/verify_navigation_and_motion.ps1` fails if the raster pass goes back to per-pixel BVH shadows, if rocks go back to a high-poly sphere, or if culling or LOD is removed.
