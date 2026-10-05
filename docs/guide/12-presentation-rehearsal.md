# Understand it, demonstrate it, defend it

Preparation workbook for Sarwad Hasan Siddiqui, CSE 4102, KUET. Prepared on 5 October 2026 for the presentation on 6 October. This is a route through the existing textbook and object references, with exercises to check understanding. The report is the submission; this workbook is for learning.

## Start here: where the resources are

| Resource | Use it for |
| --- | --- |
| [Learning-guide index](README.md) | Eleven chapters deriving the entire implementation from first principles |
| [Slide-by-slide speaker notes](../report/Presentation_Notes_Source.md) | What to explain on each of the thirteen slides, with formulas and code pointers |
| [Visual fundamentals](../report/Visual_Fundamentals.md) | Full-size diagrams, all 26 bodies and all eighteen hardware targets |
| [Object handbook](../objects/README.md) | Exact construction, transforms, material credits and limitations of each object |
| [Audit and viva examples](../report/Assessment_and_Viva.md) | Numerical examples, measured performance and the assessment checklist |
| [Controls](../objects/controls.md) | Accurate keys and camera/manual-flight behavior |
| [Change recipes](10-how-to-change-things.md) | Answering “where would you change this?” with an actual file/function |

Keep the presentation open beside this workbook. For every technical slide, follow **purpose → construction/calculation → visible result → limitation**. That gives an explanation rather than a list of features.

## A realistic preparation route

If you have three hours, use the route below. The durations are study budgets, not a promise of mastery. If a checkpoint fails, spend time on the linked chapter instead of repeatedly rereading the slide.

| Minutes | Work | Closed-book checkpoint |
| --- | --- | --- |
| 0–15 | Explain the project and one frame; sections 1–2 below | Draw CPU geometry → GPU pipeline → pixels |
| 15–45 | Geometry and spacecraft; sections 3–4 | Construct one indexed cell and derive sphere/dish counts |
| 45–70 | Transforms, cameras and motion; sections 5–6 | Transform one point and distinguish the clocks |
| 70–100 | Lights, shading and texture; sections 7–9 | Compute diffuse light; compare all five shading modes |
| 100–125 | Shadows and tracing; sections 10–11 | Solve a ray hit; explain why the BVH skips work |
| 125–140 | Performance, scope and live controls | Explain three optimizations and three limitations |
| 140–165 | Present all thirteen slides aloud | Use the pictures without reading paragraphs |
| 165–180 | Mock viva and weak-topic repair | Answer the questions at the end without opening notes |

If you only have ninety minutes: learn the opening and pipeline (10), sphere/dish/indexing (20), TRS/motion (15), lighting/shading/maps (20), tracing/optimization (15), then rehearse the demo and five hardest questions (10). Read the eighteen-part reference when asked about a particular component; learn their shared construction rules first.

For each topic, score yourself: **0** = cannot explain; **1** = can repeat a description; **2** = can explain why, solve its example and point to evidence. Revisit scores 0–1. Reading familiarity is not the same as being able to explain it.

## 1. Your opening: what did you build?

Use this as a model, then say it in your own words:

> “My project is an interactive Voyager 2 solar-system explorer built with C++20 and OpenGL. I generate the geometry procedurally, organize it through a scene graph, and combine dated mission playback with manual flight. The same objects demonstrate transformations, three light types, five shading techniques, textures, shadows and hybrid ray tracing. I can inspect the spacecraft and compare rendering techniques in controlled views.”

The central design problem is **scale**: metre-scale hardware and interplanetary distances must coexist in an explorable scene. Compressed display scales make them visible; double-precision world positions and a floating origin preserve close-up precision. Historical data and presentation mappings are different things.

The inventory is 26 bodies, four ring systems with fifteen bands, 76 spacecraft assemblies, eighteen inspection targets, 8,500 instanced rocks and a comet. The actual background has **7,380 points: 5,200 faint + 1,800 medium + 380 bright**. Earlier assessment text incorrectly said 4,000 stars; 4,000 is the asteroid instance count. Do not confuse these counts.

Explain the stack by responsibility: GLFW creates the window/context and supplies input; GLAD loads OpenGL functions; GLM supplies vector/matrix/quaternion mathematics; OpenGL draws. Your generators and scene code supply the project-specific geometry and behavior. NASA hardware imagery is used as texture, not as an imported spacecraft mesh.

**Check:** describe the purpose without naming a library. Then explain why the scene is not literally to scale. Read [Voyager](08-voyager.md) and [mission data](09-mission-data.md) if that distinction is unclear.

## 2. Follow one vertex all the way to the screen

Read [pipeline chapter](01-opengl-pipeline.md).

1. A generator produces CPU `MeshData`: positions, normals, UVs and indices.
2. `Mesh` uploads vertex bytes to a **VBO** and indices to an **EBO**. A **VAO** records the attribute layout/buffer associations; it is not another copy of the mesh.
3. Each vertex has eight floats: position xyz, normal xyz, UV. At four bytes each this is **32 bytes**. Attributes 0, 1 and 2 select those fields.
4. The vertex shader applies model, view and projection transforms.
5. Triangles are assembled and clipped. Rasterization determines covered samples and interpolates attributes.
6. The fragment shader samples materials and evaluates lighting; depth testing/blending determine the final framebuffer contribution.

A vertex is a surface record; a fragment is a candidate contribution to a pixel/sample. One vertex does not correspond to one pixel. Thousands of fragments can arise from one large triangle, and fragments can fail depth testing.

The application maintains **input → update → render**. Input changes state; update advances motion and controllers; render consumes that state. `src/scene/` describes the world; `src/rendering/` performs GPU work; `src/core/` coordinates the application.

**Exercise:** two triangles share a quad with indices `(0,1,2)` and `(1,3,2)`. Explain why this needs four records, not six. Then explain why a cube still needs 24 records although it has only eight geometric corners. Answer: a complete record includes normal and UV; hard edges require different face normals.

## 3. Reconstruct the sphere rather than memorizing its count

Read [geometry §2.1–2.2](02-geometry.md) and [sphere reference](../objects/uv-sphere.md).

![Indexed cell](../report/figures/technical/index_cell.png)

Let latitude index be `i`, longitude index `j`, latitude segments `L=32`, longitude segments `N=64`:

```text
φ = πi/L                         θ = 2πj/N
p = (sinφ cosθ, cosφ, sinφ sinθ)  n = normalize(p)
UV = (1−j/N, 1−i/L)
```

Why this works: at each latitude, `y=cosφ` and the horizontal circle has radius `sinφ`. As θ advances, cosine/sine trace that circle. On a unit sphere, the outward radial direction is also the normal.

There are 33 rows and 65 columns, hence **2,145 records**. The extra longitude column repeats a position with a different UV at the texture seam. One record cannot simultaneously hold `u=0` and `u=1`.

With row width 65:

```text
TL = i×65+j   TR = TL+1   BL = TL+65   BR = BL+1
Triangles: (TL,TR,BL), (TR,BR,BL)
```

At `i=10, j=20`, indices are `670,671,735,736`. Write `(670,671,735)` and `(671,736,735)`. Counter-clockwise outside winding selects the outward face. At the poles one half of each cell collapses, so omit those zero-area triangles:

```text
triangles = 2N(L−1) = 3,968
indices   = 3×triangles = 11,904
```

All bodies share this mesh; different textures, scales and motion distinguish Earth from Jupiter. Increasing subdivision improves silhouette smoothness but increases vertex/triangle work. Normal maps improve illumination detail without improving silhouette.

**Check:** generate a `2×4` sphere on paper: 15 records and eight nondegenerate triangles. Explain seam duplication and pole omission without looking at the answer.

## 4. Explain all objects through a small set of construction rules

Read [geometry §2.3–2.8](02-geometry.md), [Voyager chapter](08-voyager.md), and the [eighteen-part catalogue](../report/Visual_Fundamentals.md#4-all-eighteen-voyager-hardware-targets).

![Primitive profiles](../report/figures/technical/primitive_profiles.png)

| Family | How you build it | Where you use it |
| --- | --- | --- |
| Box | Four vertices per face; two triangles per face; face normals/UVs | Blankets, instrument housings, fins, panels |
| Cylinder/frustum | Two angle-sampled rings; split side quads; separate flat-normal cap fans | Bus, barrels, generators, nozzles |
| Rod | Cylinder along +Y, rotated onto `B−A`, moved to `(A+B)/2`, length `|B−A|` | Supports, antenna elements, truss diagonals |
| Parabolic dish | Sample radial rings of a paraboloid; front/back plus rim | High-gain antenna |
| Annulus | Inner/outer circular rows; separate top/bottom faces | Fifteen planetary ring bands |
| Points/lines | Positions drawn with point/line primitive modes, no triangles | Stars, orbit/path guides, heliosphere |
| Instanced rock | One low-poly sphere plus many instance matrices | Asteroid/Kuiper/Oort fields |
| Comet assembly | Nucleus sphere, additive coma and cone tail | Dynamic illustrative comet |

A capped cylinder with `n=16` and nonzero radii has `4n+6=70` vertices and `4n=64` triangles. Caps duplicate rim positions because their normal is vertical rather than radial. A taper changes the side normal: it is proportional to `(h cosθ, rb−rt, h sinθ)`, not just `(cosθ,0,sinθ)`.

The curved dish is particularly useful for defending extra curved geometry:

```text
y = −D + D(r/R)²
front normal ∝ (−2Dx/R², 1, −2Dz/R²)
```

At `r=0`, the dish is at `−D` and the normal points +Y. At `r=R`, it reaches the rim at zero. The normal comes from the derivative of the surface, so it changes continuously with slope. Actual construction uses **48 angular segments and six radial rings**:

```text
one face vertices = 1+6×49 = 295
one face triangles = 48 + 5×48×2 = 528
both faces + rim   = 590 vertices, 2×528+2×48 = 1,152 triangles
```

Assemblies transform and append primitive vertices. If the destination already has 24 records, a newly appended primitive's local index 0 must become 24. Without this index rebasing, its triangles reference the first primitive's vertices.

Explain hardware by family: bus/cameras/RTGs use cylinders; instrument housings use boxes; booms use trusses of rods; antenna uses a dish; thrusters use frustums. The catalogue names all eighteen inspection targets and their exact combinations. **76 assemblies, eighteen targets and 5,828 triangles measure different things.**

For stars, uniform θ and uniform `cosφ` avoid polar clustering. The three rock fields use 4,000/3,000/1,500 instances of a 28-record, 24-triangle sphere. Their placements are static. The comet moves and points its tail away from the Sun, not necessarily opposite its velocity.

**Check:** given any hardware close-up, identify its primitives, normals, local transform and material. Explain why the NASA foil image adds wrinkles without adding triangles.

## 5. Make matrices tangible

Read [transforms/cameras](03-transforms-and-cameras.md).

![Hierarchy](../report/figures/technical/transform_hierarchy.png)

For column vectors:

```text
local matrix = T R S
world matrix = parentWorld × local
clip point   = projection × view × world × localPoint
```

Read the operations right to left. Scaling changes size around the local origin; rotation changes orientation; translation places the object. A parent carries all its children, so one Voyager root quaternion/position moves every assembly.

**Calculate:** `(1,0,0)` scaled by two becomes `(2,0,0)`. A +90° Y rotation gives `(0,0,−2)`. Translation `(10,0,0)` gives `(10,0,−2)`. Parent translation `(0,3,0)` gives **`(10,3,−2)`**. If you rotate the translation too, the result differs.

Normals need `normalize(transpose(inverse(M3×3)) × n)`. Why? If `n·t=0`, transformed normals must remain perpendicular to transformed tangents. With scale `(2,1,1)`, tangent `(1,−1,0)` becomes `(2,−1,0)`. Inverse-transpose sends normal `(1,1,0)` to `(.5,1,0)`; their dot product is zero. Direct scaling gives `(2,1,0)` whose dot product is three: wrong.

A planet's sphere spins, but moons/rings inherit the **unspun tilted frame**, preventing daily spin from dragging their orbits. The floating origin subtracts camera position in double precision before sending float translations to the GPU. Logarithmic depth separately addresses depth-buffer precision; these solve different problems.

Four cameras: FreeFly moves independently; Focus orbits a body; Chase follows the spacecraft frame; Inspect frames hardware in that frame. Changing a camera changes observation, not object geometry.

**Check:** sketch which matrix spins the surface and which matrix a moon inherits. Explain why ordinary float coordinates become unsuitable for tiny hardware far from the origin.

## 6. Distinguish historical motion, illustrative motion and manual control

Read [mission data](09-mission-data.md), [orbital motion](../objects/orbital-motion.md) and [Voyager flight](../objects/voyager-2.md).

| Motion | Its input | Meaning |
| --- | --- | --- |
| Dated planets and Voyager | Offline Horizons positions/velocities, one Julian Date | Sourced historical trajectories inside the table interval |
| Moon revolution and axial spin | Visual rates, shared speed/pause | Educational animation, not dated lunar ephemeris |
| Comet | Illustrative ellipse and Sun-relative tail direction | Dynamic demonstration |
| Camera/manual flight | Real frame time and user input | Still usable while astronomical motion is paused |

Hermite interpolation uses positions **and velocities**. With normalized interval parameter `s` and duration `h` in days:

```text
p(s) = (2s³−3s²+1)p0 + (s³−2s²+s)h v0
     + (−2s³+3s²)p1 + (s³−s²)h v1
```

At `s=0` it equals `p0`; at `s=1` it equals `p1`. Velocity is multiplied by `h` to convert AU/day into the interval's displacement scale. At the midpoint the weights are `.5,.125,.5,−.125`. For scalar `p0=0,p1=10,v0=v1=1,h=10`, the result is **5**. Curved data with unequal velocities changes that midpoint and avoids simple piecewise-linear corners.

After 2030, planets use two-body Kepler continuation and Voyager coasts ballistically; these are predictions, not additional NASA measurements. Display compression and flyby clearance further alter visible geometry.

Manual flight updates velocity with acceleration, then position with velocity. Turning changes facing, not existing momentum. Quaternion rotations avoid Euler gimbal lock, but normalization is still needed to limit numerical drift. Braking opposes velocity without overshooting; speed is capped at twelve render units/s.

**Check:** explain why a turned spacecraft can keep moving sideways. Press `P`, then orbit the camera: identify exactly what stops and what continues.

## 7. Calculate the light before discussing the image

Read [lighting](05-lighting.md).

The normal `n` describes the surface. The unit vector `l` points toward a light. Lambert diffuse is `D=max(n·l,0)`: face-on gives 1, 60° gives .5, and light behind the surface gives zero.

| Light | Direction at the surface | Extra factor | Project example |
| --- | --- | --- | --- |
| Point | `normalize(lightPosition−surfacePosition)` | Distance attenuation | Sun |
| Directional | Same `−lightDirection` everywhere | No positional falloff | Cool fill |
| Spot | Point-light direction | Distance attenuation and angular cone | Camera headlamp |

Point attenuation is `1/(kc+kl d+kq d²)`, with a small positive denominator guard. The optional Sun law is `1/(.3+.7(d/20)²)`: at distance 20 it is 1, at 40 it is about **.323**. Default constant intensity preserves readable distant planets.

The spotlight compares the light's forward direction to the direction from light to surface:

```text
C = clamp((cosθ−cos18°)/(cos12°−cos18°),0,1)
```

At 12° or inside, full cone intensity; at 18° or beyond, zero; between, smooth transition. The minus sign on `l` matters because `l` points surface→light. The headlamp's position/direction follow the camera, creating moving-light behavior when the camera moves.

Ambient prevents every unlit region from becoming fully black. Specular depends on the viewer as well as light/normal. Sun visibility attenuates Sun terms while fill/headlamp remain independent; neither auxiliary light casts shadows. Inspect adds a camera-related studio fill, so comparisons must acknowledge that mode.

**Check:** sketch point/parallel/cone rays. Explain why `F5` changes a dark Moon and why `F7` can make distant scenes dimmer without changing their geometry.

## 8. Separate lighting from the five shading techniques

Read [shading chapter](06-shading-techniques.md).

Lighting answers where illumination comes from. Shading answers which normal/material model is evaluated and where it is evaluated.

| Mode | Calculation location and normal | What to look for |
| --- | --- | --- |
| Flat | Per-fragment lighting using one geometric face normal | Faceted triangular changes |
| Gouraud | Lighting at vertices; interpolate its terms | A highlight between vertices can disappear |
| Phong | Interpolate/normalize normals; evaluate reflection-vector lighting per fragment | Smooth surface with view-dependent highlights |
| Blinn–Phong | Per-fragment halfway-vector specular | Similar smooth result with a different specular calculation |
| Toon | Banded diffuse, thresholded specular and rim darkening | Discrete bands and ink-like rim |

```text
Phong: r = 2(n·l)n−l             specular = ks max(v·r,0)^p
Blinn: h = normalize(l+v)        specular = ks max(n·h,0)^(2p)
```

The exponent controls highlight width: `.8^10≈.107`, `.8^40≈.000133`. Higher power suppresses less-aligned directions and concentrates the highlight. `2p` is this implementation's approximate Blinn highlight-size match, not a universal identity.

Toon positive-diffuse thresholds `.75/.40/.12` map to `1/.62/.30/.06`. Gouraud disables derived normal maps because its lighting is evaluated before fragment detail is available. Texture detail can hide shading differences on a dense sphere; use the matched spacecraft views too.

**Check:** answer “why can Gouraud miss a small specular highlight?” using a triangle with a bright center and dark vertices. Explain the difference between **Phong shading** and the **Phong lighting model**.

## 9. Texture detail is not mesh detail

Read [textures](04-textures.md).

UV is a two-dimensional address into an image. Albedo supplies surface colour. Mipmaps provide reduced-resolution levels for minification; filtering smooths sampling. They do not add vertices.

The spacecraft uses a NASA hardware **atlas**. Each material selects a rectangle using `uv'=uv×scale+offset`; eleven textured finishes share the atlas, with copper untextured. Geometry remains authored in the builder.

Derived relief starts from image luminance `H=.2126R+.7152G+.0722B`. Sobel kernels estimate horizontal/vertical gradients, producing `normalize(−s Sx(H),−s Sy(H),1)`. Components are encoded from `[-1,1]` to `[0,1]`. Fragment derivatives reconstruct a tangent frame; the sampled normal changes the light response, not the silhouette. Colour-derived relief is an approximation, not measured topography.

Earth's specular mask gives stronger response to blue-dominant dark pixels, approximating ocean reflectivity. `F8` controls normal/specular maps together; demonstrate Moon relief and Earth water highlights separately if asked to distinguish their effects. In the F9 view, only albedo is carried over: raster normal/specular maps are omitted.

**Check:** predict what happens when a normal map is enabled on a sphere seen edge-on. Answer: interior lighting changes; its geometric outline remains spherical.

## 10. Shadows test visibility; ray tracing can also generate the view

Read [ray-tracing chapter](07-ray-tracing.md).

![Ray paths](../report/figures/technical/ray_paths.png)

In raster mode, triangle rasterization generates visible fragments, then shadow calculations ask whether Sun light reaches them. Analytic spheres/rings block or attenuate light; a finite Sun gives penumbrae. Voyager's raster self-shadows use a **2048² depth map** from the Sun, comparing depths with bias and averaging 3×3 PCF samples for soft mode. PCF filters visibility; it is not a bounce-light solver.

In F9, primary rays instead find the nearest analytic body/ring or Voyager triangle. The environment still rasterizes: this is a **hybrid Whitted renderer**. F10 toggles one optional reflection bounce. Four ring layers permit bounded transparency. Reflections omit Voyager; there is no refraction or diffuse global illumination.

For unit ray direction, `p(t)=o+td`. Sphere substitution yields:

```text
b=(o−c)·d        f=|o−c|²−R²
t=−b±sqrt(b²−f)
```

**Calculate:** `o=(0,0,0), d=(0,0,−1), c=(0,0,−5), R=1`. Then `b=−5,f=24`; hits are **4 and 6**, so the visible front intersection is four units away. A negative discriminant misses. If inside the sphere, choose the positive exit.

A ring first intersects a plane: `t=((c−o)·N)/(d·N)`, then checks whether radial distance is between inner/outer radii. Reject parallel rays and nonpositive distances. Reflection uses `dr=d−2(d·n)n`; bias the origin to prevent immediately re-hitting the same surface.

**Check:** distinguish `F4` visibility changes from `F9` primary-ray rendering. Explain why a black shadow and a reflection are different calculations.

## 11. Why a BVH makes spacecraft tracing practical

Voyager has 5,828 triangles. Testing every triangle for every primary/shadow ray would repeat thousands of tests even through empty space.

A **bounding volume hierarchy** encloses groups of triangles in boxes. If the ray misses a box, reject every triangle under it. Traverse near children first; a close hit shortens the maximum distance and prunes farther boxes. Leaves test at most four triangles. The builder uses twelve-bin surface-area-heuristic splits; the current tree has 6,723 nodes and depth 24, with a 32-entry traversal stack.

Triangle tests solve `o+td=A+u(B−A)+v(C−A)`. Möller–Trumbore computes `t,u,v`; accept positive `t`, nonnegative `u,v`, and `u+v≤1`. Barycentric weights `(1−u−v,u,v)` also interpolate normals and UVs. For `u=.2,v=.3`, the weights are **`.5,.2,.3`**.

Read the BVH/intersection portions of [chapter 7](07-ray-tracing.md), then inspect `TriangleBvh.cpp` and `shaders/raytrace_mesh.glsl`. Know the purpose before trying to memorize every cross product.

**Check:** draw two nested boxes and a ray that misses one. Explain the skipped work and why a BVH does not mean every ray tests exactly one triangle.

## 12. Defend optimization and scientific scope

Read [performance](11-performance.md).

| Optimization | Saved work | Tradeoff/limit |
| --- | --- | --- |
| Shared meshes | Duplicate geometry storage/uploads | Every instance still needs transforms/materials |
| Instancing | Thousands of CPU draw submissions | `8,500×24=204,000` rock triangles still exist |
| Sphere LOD | Distant vertex/triangle work | Coarser silhouette, chosen by apparent size |
| Frustum culling | Off-screen rendering | Objects still require relevant simulation updates |
| Texture detail | Tiny modeled surface features | Relief does not change silhouette |
| BVH | Unnecessary triangle intersection tests | Traversal/storage/build cost remains |
| Voyager shadow map | Repeated full BVH shadow traversal in raster view | Resolution, bias and filtering artifacts |

The recorded eight-view Release benchmark is one RX 590 run at 1440×900 with vsync disabled: mean **1.71 ms**, raster **.64–1.15 ms**, traced Voyager **6.60 ms**. Say these are measured timings, not guaranteed FPS on the teacher's computer. Frame interval and GPU render timing measure different things. Historical optimization comparisons were not re-measured for the report rebuild.

Scientific limits: compressed sizes/distances and flyby clearance; illustrative moon/spin/comet behavior; static fields; approximate fixed spacecraft hardware; bounded tracing; post-table predictions. Explaining these precisely demonstrates understanding rather than weakening the project.

## A live rehearsal that follows the slides

Run from the repository root with `scripts/run.ps1 -NoBuild`. Rehearse this yourself; scripted screenshots do not prove you can operate every control. Press `F1` whenever unsure. Check the HUD before each comparison because toggles retain state.

| Step | Action | Explain while showing it |
| --- | --- | --- |
| Overview | `H`, `T` if the path is hidden; show your video after slide 2 | Scope, compressed scale and mission path |
| Mission | `2`–`5`, check pause/rate before letting time advance | One date drives planets and spacecraft |
| Bodies/moons | Tab to a planet, brackets for its moons, RMB orbit/wheel zoom | Shared sphere, texture, tilt, orbit versus spin |
| Hardware | `I`, then `.` / `,` | Eighteen targets built from reusable primitives |
| Motion | Pause with `P`, move/orbit camera, resume | Astronomical pause leaves camera live |
| Manual flight | Ensure Chase, press `V`; thrust `W`, yaw `A/D`, pitch `R/F`, roll `Q/E`, brake `X`; `V` restores Historical | Facing and velocity are different; quaternion control |
| Lights | Focus the Moon (Tab to Earth, then `]`); pause; compare `F5/F6`, move the camera with headlamp on | Point/spot/directional geometry and moving light |
| Shading/maps | `F9` must show raster; cycle `F3`; compare `F8` | Evaluation stage, normals and texture-derived detail |
| Shadows | Focus Saturn; `F4` cycles off/hard/soft | Visibility, ring attenuation and finite-light softness |
| Tracing | `F9`; compare `F10`; inspect Voyager too | Primary hits, BVH and bounded reflection |

F1/help may consume the first Escape; Inspect can consume another. Read the HUD instead of relying on a memorized global key state. Auxiliary lights can obscure the Sun-shadow comparison, so explicitly establish the light state. In manual flight the controls apply to the ship only in Chase; in free flight, movement keys move the camera.

If live operation fails, use your recording and the matched images in [visual fundamentals](../report/Visual_Fundamentals.md). Explain the calculation and the actual screenshot; do not invent an unverified behavior to fill a gap.

## A thirteen-slide speaking route

These are prompts, not paragraphs to memorize. Use [full speaker notes](../report/Presentation_Notes_Source.md) to expand them.

| Slide | Say/show | Bridge to the next slide |
| --- | --- | --- |
| 1 | Purpose, authored spacecraft, solar-system scope, your details | “Here is what the user can actually do.” |
| 2 | Mission, inspection, controls, comparison; then your video | “Now I will explain how those images are built.” |
| 3 | Vertex record, sphere equations, one indexed cell, seam/poles | “The spacecraft combines other reusable shapes.” |
| 4 | Box/cylinder/dish construction, normals, merge/rebase, hardware | “The same scene also contains rings and environmental objects.” |
| 5 | Annuli, points/lines, instancing, comet | “Geometry needs transforms to reach its correct position.” |
| 6 | TRS, hierarchy, floating origin, cameras | “Time and input then change those transforms.” |
| 7 | Hermite, moon/spin frames, inertial quaternion flight | “Once placed, surfaces must receive illumination.” |
| 8 | Three light types, cone/falloff, moving headlamp | “The same lights can be evaluated in different shading techniques.” |
| 9 | Five methods, evaluation stages, specular differences | “Light also needs a visibility test.” |
| 10 | Shadows, ring transparency and spacecraft depth map | “Ray tests can generate visible surfaces too.” |
| 11 | Sphere/ring/triangle hits, BVH, one bounce, hybrid limits | “The visible surface also samples its material images.” |
| 12 | Albedo, atlas, Sobel normal, specular mask | “These mechanisms complete the explorer and its comparisons.” |
| 13 | Thanks; invite questions on implementation/evidence | Open notes or code only if needed for a precise detail |

For each image, identify **what changed**, **what stayed fixed**, and **which equation predicts the difference**. For example: same Moon/camera, spotlight enabled, cone/attenuation/diffuse changed; mesh and albedo did not.

## Mock viva: answer before opening the explanation

<details><summary>Why triangles? Why indices?</summary>
Three noncollinear points define a plane and fit the GPU's triangle pipeline. Indices share complete records across neighboring triangles, reducing duplication. Attribute discontinuities still need separate records.
</details>

<details><summary>Why 24 cube vertices, not eight?</summary>
A corner belongs to three faces with different normals and often different UVs. A vertex record contains one normal/UV, so each face owns four records.
</details>

<details><summary>How does a mathematical parabola become a curved antenna?</summary>
Sample radii and angles of the paraboloid, connect neighboring records with triangles, calculate derivative-based normals, duplicate/reverse the rear surface and join the rim. Curvature is approximated by finite triangles; smooth normals improve apparent continuity.
</details>

<details><summary>Why not rotate the entire planet parent to create daily spin?</summary>
That would rotate its moons/rings too. The unspun parent carries tilt/position/scale; spin is applied to the sphere surface alone.
</details>

<details><summary>What is the difference between view and model transforms?</summary>
Model places an object in the scene; view expresses the scene relative to the camera. Projection maps that camera space into clip space. Changing the view does not rebuild the model.
</details>

<details><summary>Why inverse transpose for normals?</summary>
It preserves their perpendicularity to transformed tangents, including nonuniform scale. A directly transformed normal can tilt incorrectly and produce wrong lighting.
</details>

<details><summary>Why Hermite instead of just linear interpolation?</summary>
The data contains velocity as well as position. Hermite uses both endpoint derivatives for a smooth curved interval; linear interpolation only connects positions and can create velocity discontinuities between intervals.
</details>

<details><summary>Is all motion physically accurate?</summary>
No. Within-table planets/Voyager use sourced dated states; visual scales, clearance, moon/spin/comet rates and post-table continuation have explicitly documented approximations.
</details>

<details><summary>Is Phong shading the same as the Phong reflection model?</summary>
No. Phong shading interpolates normals for per-fragment evaluation. The Phong model defines diffuse/specular response with the reflection vector. Gouraud can evaluate a Phong light model at vertices.
</details>

<details><summary>Why can moving the eye change a highlight?</summary>
Specular uses the view direction as well as normal/light. Diffuse at a fixed surface/light does not inherently depend on the eye. A camera-mounted headlamp also moves the light itself.
</details>

<details><summary>Does a normal map add vertices?</summary>
No. It changes the shading normal sampled per fragment. It does not modify geometric position or silhouette; this implementation infers relief from colour rather than measured height.
</details>

<details><summary>Does a ray-traced shadow mean the whole image is ray traced?</summary>
No. Raster mode uses ray-based Sun visibility while primary visibility remains rasterized. F9 uses primary rays for bodies/rings/Voyager, but the environment still rasterizes.
</details>

<details><summary>Why BVH instead of testing every spacecraft triangle?</summary>
Bounding boxes reject whole groups without triangle tests. Near-first traversal and current closest distance prune more work. This accelerates intersection queries while preserving the actual triangle surface.
</details>

<details><summary>Is the F9 renderer a path tracer?</summary>
No. It is a bounded hybrid Whitted view with one optional reflection, no diffuse bounce/refraction/global illumination and no Voyager in secondary reflections.
</details>

<details><summary>Does instancing reduce the triangle count?</summary>
No. It reduces repeated draw submission and geometry copies. Triangle work depends on the shared mesh and instance count; low-poly meshes/LOD address that separately.
</details>

<details><summary>Where would you change the spotlight cone, a finish or a sphere resolution?</summary>
Cone: LightingController::build. Finish/atlas rectangle: VoyagerModelBuilder and material settings. Sphere tessellation: generator parameters/shared LOD meshes. Read chapter 10; build and inspect relevant changes rather than guessing.
</details>

## Final readiness check

Without notes, draw an indexed cell, the planet hierarchy and a primary/shadow/reflection diagram. Solve the TRS, diffuse, Hermite-midpoint and sphere-hit examples above. Explain all five shading modes, three light types, texture versus geometry and raster versus traced visibility. Identify a code location for each. Then present the entire deck aloud once and perform the control sequence once.

If you can do that, you have evidence of understanding. If you cannot, use the failed explanation to choose the chapter to revisit. Keep precise limitations in your answers and your actual project files available for detailed questions.
