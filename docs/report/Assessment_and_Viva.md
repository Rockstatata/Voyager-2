# Requirement audit, fundamentals and viva preparation

Audited on 5 October 2026 for **Sarwad Hasan Siddiqui, Roll 2107006**, CSE 4102: **Computer Graphics and Image Processing Laboratory**. Course teachers: **Md Tajmilur Rahman, Lecturer** and **Md Mubtashim Abrar Nihal, Lecturer**.

Read the [nineteen-page LaTeX report](Voyager_2_Graphics_Report.pdf) for submission and this document for rehearsal. The [thirteen-slide presentation](Voyager_2_Presentation.pptx), [detailed speaker notes](Presentation_Notes_Source.md) and [visual fundamentals](Visual_Fundamentals.md) connect the equations to runtime images. The [learning guide](../guide/README.md) gives the full derivations; the [object index](../objects/README.md) explains each object. Screenshots do not establish every input behavior.

## 1. What the supplied note requires

The user clarified the handwritten outline: **two introduction slides**, **ten detailed technical slides**, and **one thank-you slide**, with the author's video shown after the introduction. The technical slides require screenshots, fundamentals and equations for objects, curved surfaces, transformations/motion, controls, textures, three light types, moving lights, shading and ray tracing. Directional light is demonstrated alongside point and spot. Curved geometry includes spheres, cylinders and the parabolic dish; the project does not claim a Bezier/NURBS implementation.

| Requirement | Code evidence | Runtime demonstration | Audit conclusion |
| --- | --- | --- | --- |
| Authored objects | `UvSphereGenerator`, `VoyagerModelBuilder`, `SolarSystemBuilder` | Tab; I; component stepping | Present; 26 bodies, 76 spacecraft assemblies |
| Curved geometry | `ParabolicDishGenerator`, `CylinderGenerator`, `RingGenerator` | I, then period twice for antenna | Present; analytic curved surfaces triangulated |
| Texture | `Texture2D`, `MaterialLibrary`, `SurfaceMaps`, `scene.vert/.frag` | Body close-ups; F8; Inspect | Present; credited images, no imported geometry |
| Complex motion | `Trajectory`, `MissionController`, `CelestialBody`, `Voyager2` | Bookmarks 2-5; P; speed; V | Present; dated and educational motion separated |
| Dynamic objects | `Comet::update`, body spin/orbits, manual flight | Unpause; follow Voyager; inspect comet | Present; comet tail follows Sun-relative direction |
| Controlling | `Input`, `CameraController`, `Camera`, `Application::handleKeys` | C; RMB; wheel; Tab; brackets; V | Present; four camera modes plus manual flight |
| Point light | `LightingController::build`, `lights[0]` | Sunlit Earth/Moon; F7 attenuation | Present; position-dependent direction |
| Spot light | `LightingController::build`, `lights[1]` | F5 near Moon | Present; 12-degree inner / 18-degree outer cone |
| Directional light | `LightingController::build`, `lights[2]` | F6, night side | Present; constant incident direction |
| Moving light | Headlamp position/direction rebuilt from camera each frame | F5, RMB orbit or free flight | Present; demonstrated by the moving-spot demo chapter |
| Ray tracing | `raytrace*.glsl`, `RayTracer`, `TriangleBvh` | F9 and F10 | Present; analytic bodies/rings and triangle spacecraft |
| Clear overall explanation | LaTeX report, visual catalogue, detailed speaker notes | Two high-level introductions and ten technical slides | Rebuilt with mathematical and visual explanations |
| Report below 20 pages | XeLaTeX source and export guard | Nineteen pages including cover, front matter and references | Template typography retained; short lists and Chapters V?VII share a page |
| Thirteen-slide structure | `build_assessment_slides.py`; editable notes source | Exactly 2 introductions + 10 technical + 1 thanks | Follows the user's clarified outline |
| Video after introduction | Cue on slide 2 | Author will record and attach the final video | No new recording or embedding; earlier MP4 retained separately |

No missing graphics category from the supplied photo was found. The missing work was the assessment package and a repeatable moving-light demonstration. A runtime input test also exposed missed short taps when press/release happened between frame polls; `Input::attach` now enables GLFW sticky keys and mouse buttons. Existing optimized rendering implementations were retained. This audit corrected the old 21-texture manifest to 26 and documentation that incorrectly guaranteed nonblocking benchmark query reads.

## 2. From a vertex to a pixel

**Vertex:** a record describing a point on a surface, not a pixel. It contains position (3 floats), normal (3), UV (2): 32 bytes. **Index:** an integer identifying a vertex. Three indices define a triangle; sharing indices avoids repeating identical vertex records. Hard edges and texture seams may still need duplicate records because their normals or UVs differ.

**VBO** stores vertex bytes; **EBO** stores indices; **VAO** records how to interpret the attributes and which buffers supply them. Mesh owns these handles through RAII. `Vertex` and `scene.vert` agree on locations 0/1/2. Instancing reserves 3-6 for four matrix columns, advancing those attributes once per instance instead of once per vertex.

For column vectors:

```text
world = parentWorld × translation × rotation × scale × local
clip  = projection × view × world
NDC   = clip.xyz / clip.w
```

The GPU transforms vertices, assembles triangles, clips them, determines covered samples and interpolates attributes. The fragment shader shades each covered sample; depth testing resolves visibility. Transparent ring/glow submissions blend after opaque objects with depth writes disabled.

**Worked transform:** local point (1,0,0), uniform scale 2, 90-degree rotation about +Y, then translation (10,0,0) produces (10,0,-2). Parent translation (0,3,0) produces world (10,3,-2). Reversing translation/rotation changes the result: multiplication order matters.

Normal direction transforms by `transpose(inverse(linearModel))`. For a surface with tangent t and normal n, n·t=0; after transforming t by M, the inverse transpose preserves that orthogonality. Normalize afterward. Simply using M for normals is wrong under non-uniform scale.

**Large-world precision:** retain positions as doubles; subtract the eye in double; only then upload floats. At an eye position of 1,000,000 units, a nearby detail 0.001 units away becomes a small camera-relative difference instead of two rounded large floats. Logarithmic depth addresses depth-buffer distribution, a separate problem from position precision. Read [guide chapter 3](../guide/03-transforms-and-cameras.md).

## 3. Build and defend the curved objects

For sphere latitude index i and longitude j, with L=32 and S=64:

```text
theta = pi i/L          phi = 2 pi j/S
p = (sin(theta) cos(phi), cos(theta), sin(theta) sin(phi))
n = normalize(p)       uv = (1-j/S, 1-i/L)
```

At i=16,j=0, p=(1,0,0), n=(1,0,0), UV=(1,0.5). At i=16,j=16, p=(0,0,1), UV=(0.75,0.5). Seam endpoints j=0 and j=64 have the same position but u=1 and u=0. One vertex cannot carry both values; duplicate the seam to avoid averaging across the whole image.

For a normal cell, TL=i(S+1)+j; TR=TL+1; BL=TL+(S+1); BR=BL+1. Triangles are (TL,TR,BL) and (TR,BR,BL). At the poles, one triangle per wedge would have coincident points and zero area; omit it. Counts are `(L+1)(S+1)=2145` vertices and `2S(L-1)=3968` triangles. The same formula gives 960 triangles for 16×32 and 224 for 8×16. LOD changes triangle count without changing the texturing convention.

For the dish, let aperture radius be R, center-to-rim bowl depth d and radial coordinate r:

```text
x=r cos(phi)       y=-d+d(r/R)^2       z=r sin(phi)
slope=2 d r/R^2
front normal=normalize(-slope cos(phi), 1, -slope sin(phi))
```

At r=0, y=-d and the normal points +Y. At r=R, y=0 and the slope is 2d/R. At r=R/2, y=-3d/4. Use a center fan, two triangles per radial/angular cell, a rear surface with reversed normals/winding, and a joined rim. Voyager uses 48 angular segments and six radial rings. This retains curvature and silhouette without the old higher-resolution mesh.

An annulus samples circles at two radii. Each angular sector joins its inner/outer endpoints with two triangles. Its hole is real empty geometry. A cylinder needs side normals and separate cap normals at shared positions; reusing side normals for the cap would round the lighting incorrectly. Full reconstruction: [mesh handbook](../objects/procedural-meshes.md).

## 4. Texture is not geometry

An image maps a colour to a UV location. UVs are interpolated across each raster triangle. Texture pixels do not create mesh vertices; a crater photograph alone cannot change a planet's silhouette. The project keeps silhouette-producing hardware as geometry while louvres, foil wrinkles and other fine detail use photographs.

Atlas mapping is `uvAtlas=uvLocal*scale+offset`. **Example:** a normalized atlas window beginning at (0.25,0.50) and spanning (0.10,0.20) maps local (0,0) to (0.25,0.50) and local (1,1) to (0.35,0.70). This lets many surfaces share the same texture allocation.

Mipmaps store progressively smaller images. Trilinear filtering chooses and blends nearby levels when an image shrinks on screen, reducing shimmering and wasted sampling detail. Longitude repeats and latitude clamps. The ray tracer stores body maps as 26 layers in a 1024×512 texture array; it chooses a layer using the hit body's index.

Normal maps approximate small orientation changes. `SurfaceMaps` estimates height by luminance, smooths gradients with Sobel kernels and stores `normalize(-strength*Gx,-strength*Gy,1)` encoded from -1…1 into 0…1. A flat patch gives encoded (0.5,0.5,1). The shader derives a tangent frame from position and UV derivatives, then rotates the sampled tangent-space normal into world-relative coordinates.

These maps are not physical terrain: a painted dark patch can look low even when the real surface is flat. Earth's ocean mask uses colour thresholds, so it is also a heuristic. F8 compares maps on/off; Gouraud cannot use these fragment perturbations for its per-vertex light terms. Credits and filenames: [body asset manifest](../../assets/textures/bodies/README.md), [spacecraft manifest](../../assets/textures/spacecraft/README.md).

## 5. Motion: data, interpolation and animation

An ephemeris maps time to a body's state. The project vendors position/velocity CSVs rather than querying a network during class. `SimulationClock` owns the mission date; `MissionController` samples every planet and Voyager using that date. The date advances by real frame time multiplied by selected speed and encounter slowdown. It is clamped at a finite upper limit (2500), not an unbounded simulation.

Hermite interpolation uses normalized u=(t-t0)/(t1-t0) and four basis functions:

```text
h00=2u^3-3u^2+1      h10=u^3-2u^2+u
h01=-2u^3+3u^2       h11=u^3-u^2
p=h00 p0+h10 Δt v0+h01 p1+h11 Δt v1
```

At u=0, only p0 remains; at u=1, only p1 remains. Derivatives at the endpoints reproduce their velocities. **Unit check:** positions in AU and velocities in AU/day require Δt in days. Multiplying AU/day by seconds would be a major error.

Moon motion is a separate educational local orbit, not a moon Horizons track. `r=a(1-e²)/(1+e cos(nu))` supplies elliptical shape, but nu advances at a constant visual rate instead of satisfying Kepler's equal-area law. Spin is accelerated and only rotates the body's own surface; children inherit axial tilt without inheriting daily spin. This avoids dragging every moon around once per day. Comet tail direction is the anti-Sun vector, recalculated as the nucleus moves.

After the 2030 table boundary, planets continue with two-body Kepler propagation; Voyager uses its continuation. This preserves motion but sacrifices multi-body accuracy. Startup comparisons already show substantial accumulated error, so do not describe these future positions as NASA predictions. [Guide chapter 9](../guide/09-mission-data.md) covers the dated data; [performance chapter](../guide/11-performance.md) covers continuation.

Manual piloting integrates velocity and position with frame delta-time. A normalized quaternion represents attitude, and local turn increments multiply on the right. Three orientation axes plus three-dimensional position give six-degree-of-freedom state. W/S supplies forward/back acceleration, Space/Ctrl vertical thrust, and the ship can reorient to thrust in another direction. Controls are pedagogical, not a real propulsion simulation.

## 6. Explain the three lights numerically

Lambert diffuse is `max(n·l,0)`: full at 0 degrees, half at 60 degrees and zero at 90 degrees or beyond. **Point:** l changes with surface-to-light vector. **Directional:** l is the negative of one constant travel direction. **Spot:** a point-like source also restricts light to a cone.

Distance attenuation is `1/(kc+kl*d+kq*d²)`. The optional Sun mapping uses `kc=0.3`, `kl=0`, `kq=0.7/400`. At d=20, attenuation is 1; at d=40 it is about 0.323. This is a display-scale model, not literal solar inverse-square irradiance. Default falloff is off so distant planets remain readable.

The spotlight computes theta=`dot(-l,spotDirection)` and clamps `(theta-cos18°)/(cos12°-cos18°)`. A point on the axis has theta=1 and full cone weight. At 18 degrees it is zero; at 12 degrees it is one. The fade is linear in cosine, not in the angle itself. Headlamp reach scales with nearest surface distance to remain useful near a small spacecraft and a large planet.

Phong specular compares eye direction with reflected incident light; Blinn compares the normal with the eye/light half vector. The code doubles the Blinn exponent to roughly match highlight width. Diffuse colour is multiplied by albedo; the highlight is added independently. Ambient 0.07 is a constant approximation, not bounced light. Sun shadow visibility multiplies only the Sun's terms; it does not darken the headlamp/fill contributions.

**Moving light proof:** `LightingController::build` assigns headlamp.position=camera.position and headlamp.direction=camera.forward each render. Enable F5 and move/orbit the camera. The pool of illumination moves because the light transform changes, not because the object material is replaced.

## 7. Shading, shadows and ray tracing are different decisions

**Flat:** triangle normal from screen-space derivatives. **Gouraud:** evaluate light terms at vertices, interpolate the results. **Phong shading:** interpolate the normal, normalize and evaluate the reflection-vector highlight per fragment. **Blinn-Phong:** fragment half-vector highlight. **Toon:** quantize diffuse bands, threshold specular and darken the rim. F3 cycles the raster techniques. The F9 tracer always uses Blinn-Phong, so F3 is not a ray-tracer comparison tool.

A shadow map records nearest depth from the light. In the camera pass, project a surface point into the map and compare its depth with the stored depth. A nearer stored depth means an occluder. Polygon offset reduces self-shadow acne; excessive bias can detach shadows. Voyager uses an orthographic light camera fitted to its bounding sphere because Sun rays are effectively parallel across the craft. Hard mode takes a comparison sample; soft mode averages nine PCF taps. Filtering smooths edges but is not global illumination.

The raster planet/ring shadows use analytic rays toward the Sun. Spheres estimate solar-disc overlap for soft shadows; rings reduce transmittance. Voyager's raster self-shadows deliberately use the depth map to avoid thousands of per-pixel BVH tests. When F9 is enabled, primary/visibility rays use Voyager's actual triangles.

For a ray `P(t)=O+tD`, substitute into `|P-C|²=r²`. With unit D, `b=(O-C)·D`, `c=|O-C|²-r²` and roots are `-b±sqrt(b²-c)`. **Example:** O=(0,0,0), D=(0,0,-1), C=(0,0,-5), r=1: b=-5,c=24; roots 4 and 6. The visible entry is t=4. A negative discriminant means a miss. If the eye is inside the sphere, the nearer root is negative, so use the positive exit.

The ring plane uses `t=((C-O)·N)/(D·N)`. Reject near-parallel rays, negative/epsilon-close t, and radial distances outside the band. Triangle intersections solve for t,u,v in `O+tD=A+u(B-A)+v(C-A)`. Hits require u>=0,v>=0,u+v<=1. The interpolated UV is `(1-u-v)uvA+u uvB+v uvC`.

The BVH groups triangles in nested bounding boxes. A ray that misses a box skips all enclosed triangles. The builder evaluates 12-bin SAH candidates, approximately minimizing `area(left)*count(left)+area(right)*count(right)`. Near-first traversal finds a close hit early, shrinking the maximum hit distance and rejecting farther boxes. The current tree has 5,828 triangles, 6,723 nodes and depth 24; GLSL 3.30 accesses it through texture buffers, not SSBOs.

One reflection ray uses `Dreflected=D-2(D·n)n`. F10 turns that bounce on/off. Secondary rays skip Voyager, and the implementation adds a weighted reflected contribution without enforcing a fully energy-conserving BRDF. Rings accumulate up to four translucent layers. Belts/comet/guides remain rasterized; matching logarithmic depth permits compositing. No refraction, diffuse bounce or Monte Carlo sampling is implemented: call it bounded Whitted ray tracing, not physically based path tracing.

## 8. Performance: what to claim and how to verify

Instancing reduces repeated CPU draw submission: the 4,000 asteroid, 3,000 Kuiper and 1,500 Oort instances are three draws. It does not make their triangles free. At 24 triangles per rock they still submit 204,000 rock triangles. Shared sphere LOD reduces distant body cost; frustum sphere tests reject off-screen bodies, while their children are still visited because a moon can be visible when its parent is not.

LOD chooses by radius/distance. Below 0.012 it uses 224 triangles, below 0.05 it uses 960, otherwise 3,968. This is an angular proxy, not a fixed pixel threshold across all FOV/resolutions. The spacecraft is culled as a group and skipped below its apparent-size threshold. Uniform locations are cached and arrays batched. SAH/ordered traversal reduces ray work; the shadow map removes BVH rays from the usual spacecraft raster path.

The fresh Release benchmark mean is **1.71 ms** on the observed Radeon RX 590 at 1440×900. Raster views range from **0.64 to 1.15 ms**, and the traced Voyager view is **6.60 ms** in this single run. These are measured uncapped application timings, not guaranteed frame rates on other hardware. The documented 4.75→1.45 ms optimization comparison in chapter 11 was an earlier session; it was not re-created in this audit.

Frame time includes the frame interval; GPU time brackets render(), including HUD. Renderer counters exclude the depth-only spacecraft pass, full-screen ray pass and text. GPU query reads use GL_QUERY_RESULT and can wait. Do not claim that frame time measures independent CPU cost or that counters account for every triangle touched by the GPU. Build warnings and verification scope are in [validation notes](validation/README.md).

## 9. Two-minute demonstration and presentation

The earlier MP4 is a separate runtime tour, retained as an optional reference. The author will record and attach the final video after the two introduction slides. Use [Presentation_Notes_Source.md](Presentation_Notes_Source.md) for explanation and narration; the timeline below is an optional recording outline.

| Time | Feature |
| --- | --- |
| 00:00-00:08 | Whole-system overview and mission trajectory |
| 00:08-00:18 | Textured Earth with live scene motion |
| 00:18-00:28 | Curved parabolic dish |
| 00:28-00:38 | Whole spacecraft and texture finishes |
| 00:38-00:50 | Dated Jupiter encounter motion |
| 00:50-00:56 | Sun point light on Moon |
| 00:56-01:02 | Spotlight enabled |
| 01:02-01:08 | Camera/headlamp move around Moon |
| 01:08-01:14 | Directional fill |
| 01:14-01:34 | Five shading modes, four seconds each |
| 01:34-01:40 | Saturn and Sun shadows |
| 01:40-01:50 | Ray-traced spacecraft, reflection off |
| 01:50-01:54 | One reflection bounce enabled |
| 01:54-02:00 | Thanks and full controls overlay |

The tour applies existing controller/lighting APIs and updates an orbiting camera only in its moving-light chapter. The recorder captures the application's HWND through FFmpeg's GDI device, not the desktop. Encoding is external to the rendering loop. Real-time settle intervals can differ slightly under heavy load; the exported video is normalized to 3,600 frames / 120 seconds. Keep the window visible during recording.

## 10. Questions to rehearse and remaining hands-on checks

1. **Why 2,145 sphere vertices rather than 2,048?** The latitude grid includes both poles and longitude includes a duplicate seam; records carry UVs as well as positions.
2. **Why does normal mapping not change the edge of the Moon?** It changes shading directions, not vertex positions or ray geometry.
3. **Why does the Moon follow Earth without an extra translation update?** Parent/world composition carries the planet's translation; its local orbit is measured in parent-relative units.
4. **Why does Earth's spin not drag the Moon?** Spin is applied to the surface matrix, while the child hierarchy inherits tilt and parent position.
5. **Does paused time freeze the camera?** No. The mission clock and body/comet animation pause; camera update and manual spacecraft flight still use real frame delta-time.
6. **Is a spot a fourth rendering technique?** It is a light type. Any compatible shading technique can evaluate its radiance.
7. **How is the moving light implemented?** Eye position and forward vector rebuild the headlamp transform every frame.
8. **Is all of F9 ray traced?** Bodies, rings and spacecraft are traced; environmental objects are composited raster draws.
9. **Why a BVH and instancing?** BVH reduces ray intersection candidates; instancing reduces repeated draw submission. They solve different costs.
10. **Why switch Voyager's raster shadows to a map?** Measured per-pixel BVH shadow cost dominated close-ups; the depth map is cheaper while retaining visible self-shadows.
11. **Is the map soft mode physically exact?** No: PCF smooths shadow comparisons. Planetary sphere softness estimates solar-disc coverage.
12. **What is still approximate?** Compressed scale, visual moon phases/spin, derived relief, bounded/reflection-additive lighting and future continuation.

Before the classroom presentation, perform these hands-on checks from bible section 43: try mouse orbit/zoom at the closest/farthest limits; move freely near a moon and far into the outer system; test all yaw/pitch/roll directions and boost/braking; switch Manual→Historical repeatedly; pause and change speed while steering the camera; verify click picking of a body and Voyager; compare hard/soft/off shadows; toggle headlamp while moving; and check the actual lab GPU's performance. Automatic source-contract scripts and prerecorded tours do not cover every input combination, boundary condition or GPU driver.
