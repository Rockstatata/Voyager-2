# Slide 1: Voyager 2 Solar-System Explorer

Open by explaining the whole project: this is a controllable, educational three-dimensional solar-system explorer, built with C++20 and OpenGL. It combines historical Voyager 2 playback with manual spacecraft flight and a procedural hardware model. The viewer can explore a mission, inspect objects and compare graphics techniques using the same scene.

The overview establishes the connected solar system. The spacecraft picture establishes the authored geometry. Saturn establishes the interaction of a curved body, inclined rings, texture, illumination and shadows. The project contains the Sun, eight planets, Pluto and sixteen moons: 26 textured bodies. There are four ring systems containing fifteen bands, 76 spacecraft assemblies containing 5,828 triangles, eighteen named hardware inspection targets, 8,500 instanced rocks, a moving comet and 7,380 point stars. Orbit guides, the mission path, heliosphere boundaries and the HUD make those objects understandable.

NASA/JPL Horizons data supplies dated planetary and spacecraft positions. The code generates the spacecraft mesh from primitives; it does not load a NASA model. The scene compresses distances and body radii so that the objects can be seen together. State that distinction before calling the mission historical: sourced dates and directions do not mean the displayed metre-to-AU scale is literal.

Student: Sarwad Hasan Siddiqui, Roll 2107006. Course: CSE 4102, Computer Graphics and Image Processing Laboratory. Course teachers: Md Tajmilur Rahman and Md Mubtashim Abrar Nihal, Lecturers, Department of CSE, KUET. Submission: October 2026.

Sources: `Application.cpp`, `SolarSystemBuilder.cpp`, `EnvironmentBuilder.cpp`, `VoyagerModelBuilder.cpp`, and `assets/data/celestial_bodies.csv`. Report Chapters I and II.

# Slide 2: Explore, control, compare

Walk through what someone can actually do. `H` opens the overview and `T` reveals the mission path. Bookmarks `1`–`6` visit launch, the four giant-planet encounters and the heliopause. `C` enters Chase; Tab or a click focuses a body; brackets select moons. `I` enters Inspect and comma/period step through eighteen spacecraft components. FreeFly permits independent movement. `P` pauses astronomical motion, while the camera and manual flight remain usable.

The project joins seven ideas: indexed geometry creates shape; hierarchical matrices create pose; ephemeris interpolation and quaternion controls create motion; light geometry creates illumination; shading evaluates surface response; images create material detail; ray intersections establish visibility and limited reflection. The next ten slides explain those fundamentals with examples from this same application.

Video cue: stop after this slide and play the author's final recording. No new video is embedded. Attach the completed file using PowerPoint's Insert → Video → This Device, or launch it externally. The cue remains usable if a live application demonstration fails. A complete recording should cover the overview/path, flybys, bodies/rings/moons, spacecraft inspection, manual controls, pause and camera freedom, moving spotlight/fill, shading comparison, shadow modes, maps and F9/F10 tracing/reflection.

The stack is C++20, OpenGL 3.3, GLFW, GLAD and GLM. Window/input, scene ownership, camera control and rendering are separated so that changing a view does not rewrite the simulation. Explain the pictures as user activities, not as unrelated scenes.

Sources: `Application::handleKeys`, `CameraController`, `MissionController`; `docs/objects/controls.md`. Report Chapter IV.

PROPOSED SCOPE AND FINAL IMPLEMENTATION
The submitted two-page proposal is titled Voyager 2: Journey Beyond the Solar System. It planned an Earth-to-interstellar journey through Jupiter, Saturn, Uranus and Neptune, a primitive spacecraft, a visible path, starfield and heliopause. It also planned translation, rotation, scaling, composite and hierarchical transforms, planet spin, Sun lighting with ambient/diffuse/specular terms, Flat/Gouraud/Phong shading, and camera and keyboard interaction.

Point to the mission, inspection and comparison screenshots above the cards while describing the implemented feature groups. Launch bookmark 1 provides a mission restart; P pauses, =/- changes speed, C changes camera rig, T toggles the path and F3 selects the named shading technique. Explain that user input changes application state, update computes motion, and render consumes the current transforms and materials.

Additional features include 26 textured bodies, four ring systems, 76 spacecraft assemblies and 18 inspection targets, 8,500 instanced rocks, a dynamic comet and 7,380 stars. The final project adds manual six-degree flight, component inspection, a moving camera spotlight and directional fill, Blinn-Phong and Toon shading, derived surface maps, shadows and hybrid BVH ray tracing. These extend the implemented feature groups from the proposal.

Give the overview first; the following ten technical slides derive geometry, transforms, motion, lighting, shading, shadows, tracing and textures. The report methodology contains the detailed implementation and equations, and its typography follows the supplied CSE-4000 report template. Play the final project video at the existing cue, narrating each visible feature as it changes.

IMPORTANT FEATURES AND VIDEO NARRATION
Use the three screenshots above the captions as evidence. Begin with the dated mission: NASA/JPL offline positions and velocities place planets and Voyager on one shared Julian Date; interpolation gives smooth flybys. In the video, point to the running date and the four giant-planet encounters. This connects historical mission data to animation and transforms (slides 2 and 7).

Next describe the spacecraft. The mesh is authored from reusable primitives rather than an imported spacecraft mesh: boxes, cylinders, rods and a sampled paraboloid. There are 76 assemblies and 18 inspectable hardware targets. Point to the dish curvature, truss structure, component labels and NASA texture atlas. The model and inspection system connect geometry, normals, hierarchical transforms and interaction (slides 2, 4, 6 and 12).

Show user control: four camera modes, mouse selection, zoom and real-time manual six-degree flight. Floating origin subtracts camera position in double precision so hardware close-ups remain stable across large world coordinates. In the video show a selected body, then Inspect, then thrust/yaw/braking; explain that input changes state and the update computes the new pose (slides 6–7).

Show the rendering comparison in a frozen view. Toggle the moving camera spotlight and directional fill, then compare Gouraud against Phong and the other three shading modes. Identify the visible response and where illumination is computed; explain ambient, Lambert diffuse and the specular exponent rather than only naming techniques (slides 8–9).

The bonus rendering feature is hybrid Whitted ray tracing. F9 traces analytic planet/ring hits and the authored spacecraft through a triangle BVH. F10 enables one optional mirror reflection. Show a raster/traced comparison and describe primary, shadow and reflected rays, then show body/ring shadows and spacecraft self-shadow (slides 10–11).

Texture detail is another important feature: the NASA atlas supplies spacecraft finishes; colour-derived relief and an ocean mask affect normal/specular response. F8 compares maps off/on without changing geometry. Explain that normal maps change shading normals, not the silhouette (slide 12).

Finally explain the environment and efficiency: four ring systems, 8,500 deterministic rock instances in three fields, 7,380 star points and a dynamic comet. Instancing, shared sphere meshes, LOD and culling reduce drawing work. Rock fields are static; the comet and mission/body animation supply motion (slides 3 and 5).

Narrate the video using feature → calculation → visible effect. Use the two-minute plan in docs/guide/13-teacher-showcase.md. The project-specific strengths are the integration, authored geometry and inspectable comparisons; the graphics algorithms themselves are established techniques.


# Slide 3: Vertices → indices → triangles → worlds

A vertex is a surface record, not a pixel: position xyz, normal xyz and UV, eight floats or 32 bytes. A VBO stores records, an EBO stores unsigned integer references, and a VAO records how the shader attributes interpret those bytes. Locations 0, 1 and 2 hold position, normal and UV. Each three indices in `GL_TRIANGLES` select a triangle; adjacent triangles can reference the same records.

For sphere grid indices i and j, let φ=πi/L and θ=2πj/N. Position is (sinφ cosθ, cosφ, sinφ sinθ); the unit-sphere normal is the normalized position. UV=(1−j/N, 1−i/L). The north pole is (0,1,0); the equator at j=0 is (1,0,0). Reversed u gives the correct outside eastward texture orientation; v=0 is south.

With row width 65, TL=i×65+j, TR=TL+1, BL=TL+65 and BR=BL+1. Emit (TL,TR,BL) and (TR,BR,BL) with counter-clockwise outside winding. Omit the first triangle at the north pole and the second at the south pole, because those halves have coincident pole positions and zero area. Duplicate seam positions because one position must have both u=0 and u=1; sharing one UV would interpolate across the map incorrectly.

For L=32, N=64: vertices=(L+1)(N+1)=2,145; triangles=2N(L−1)=3,968; indices=11,904. Every body shares this topology. Material, radius, spin and texture distinguish the 26 bodies shown in the catalogue. Medium/low LOD copies have 561/960 and 153/224 vertices/triangles respectively. These reduce work only when the silhouette is small.

The generator validates counts, bounds, unit vectors, UV ranges, nonzero triangle area and texture direction. The wireframe is a real runtime polygon-line capture. Sources: `UvSphereGenerator.cpp`, `Vertex.h`, `Mesh.cpp`; report §3.1 and `docs/objects/uv-sphere.md`.

# Slide 4: Voyager: authored from reusable primitives

The model has 76 assemblies, 5,828 rendered triangles and eighteen inspectable components. Dimensions are authored in metres and multiplied by 0.006. NASA references guide proportions and images; the mesh is authored from boxes, cylinders, frustums, rods and a parabolic dish.

Box construction: six faces each own four vertices because face normals and UVs differ at a hard edge. A face with base index b emits (b,b+1,b+2) and (b,b+2,b+3). Counts are 24 vertices, 36 indices and twelve triangles. Scaling a shared box creates panels, fins and instrument housings.

Cylinder/frustum construction: generate two seam-inclusive rings with p=(r(k)cosθ, ±h/2, r(k)sinθ). A sloping side normal is proportional to (h cosθ, rb−rt, h sinθ). Each side quad splits into two triangles. Each enabled cap has its own center, rim vertices and flat normal. For nonzero bottom/top radii with both caps: 4n+6 vertices and 4n triangles. A cone omits the zero-radius cap and degenerate side halves. A rod starts along +Y, rotates toward B−A, has length |B−A| and midpoint (A+B)/2. An open four-sided rod has eight triangles; hidden ends need no caps.

Dish construction: y=−D+D(r/R)². Differentiating the implicit surface gives a front normal proportional to (−2Dx/R²,1,−2Dz/R²). Add the center and six radial rings of 49 seam-inclusive angular vertices. Reverse rear normals/winding and offset the rear by thickness. Join the rim. Each face has 295 vertices and 528 triangles; the rim adds 96 triangles. Total: 590 vertices, 1,152 triangles and 3,456 indices. The current model uses six rings, not the older eight-ring example.

Merged parts bake positions with M and normals with the inverse transpose; indices add the destination vertex count. A triangular truss places three rails 120° apart and alternating diagonal rods per face per bay. This lets the builder express real hardware layout while sharing a small set of mesh rules.

All eighteen inspection targets and their construction:

1. Electronics bus: a ten-sided capped cylinder, ten box-like blanket bays and adapter feet.
2. High-gain antenna: closed parabolic dish, rim-following rods and feed struts.
3. Feed stack / low-gain antenna: frustums, discs and a cone aligned with the dish boresight.
4. Sun sensor: box housing with an aperture.
5. Golden Record: cylindrical disc and hub on a bus face.
6. Optical calibration target: thin rectangular plate.
7. Magnetometer boom: canister and thirteen-metre triangular truss.
8. Low-field magnetometer: sensor boxes at the boom tip, with other sensors along the boom.
9. Radioisotope generators: boom truss, three cylindrical cores, box fins and end flanges.
10. Plasma science instrument: cylindrical housing and three cup frustums.
11. Cosmic ray subsystem: box housing and two telescope barrels.
12. Low-energy charged particles: cylindrical drum and platform.
13. Scan platform: box platform, actuator and instrument housings.
14. Narrow-angle camera: long cylindrical barrel and lens material.
15. Wide-angle camera: shorter barrel and lens.
16. IRIS telescope: wide barrel and mirror/lens finish.
17. Radio/plasma-wave antennas: two long rods in a V with a root housing.
18. Attitude thrusters: four blocks and sixteen copper nozzle frustums.

The full labelled component plate is `figures/technical/voyager_catalog.png`; individual images and limitations are in `docs/objects/voyager-2.md`. The scan platform is fixed and booms remain deployed. Source: `VoyagerModelBuilder.cpp` and the Box/Cylinder/ParabolicDish generators. Report §3.2.

# Slide 5: Rings, belts, stars, comet and scene guides

Rings are double-sided annuli, not filled discs. Generate inner and outer radius vertices at p=(r cosθ,0,r sinθ). Separate top/bottom normals and reverse the back winding. At each segment, top triangles are (innerLeft,outerLeft,innerRight) and (innerRight,outerLeft,outerRight). For 64 segments: 4(n+1)=260 vertices and 4n=256 triangles per band. Fifteen colour/opacity bands form four systems. Rings inherit planet tilt/scale but not daily surface spin.

The asteroid belt has 4,000 instances, the Kuiper belt 3,000 and the Oort cloud 1,500. Each uses a 3×6 low-poly sphere: 28 vertices and 24 triangles. Per-instance matrices at attributes 3–6, with divisor one, provide placement, random rotation and nonuniform size. Three instanced draws replace 8,500 separate draws. The fields are static; do not describe them as individually orbiting. The shell uses cube-root interpolation of radius cubed for uniform volume; the annulus uses an illustrative outward-biased square-root placement rule.

The 7,380 star points sample azimuth uniformly and cosφ=2b−1 uniformly, avoiding polar clustering. They use `GL_POINTS`, so there are no triangles. The background is centered on the camera. Circle meshes contain 96 line vertices and no triangles. Planet orbit guides sample dated trajectories; mission paths use sampled line strips. Each heliosphere boundary uses three orthogonal great circles, six loops total, representing Voyager's 84 AU and 119 AU crossing distances.

The dynamic comet is a container with a sphere nucleus, larger additive glow coma and a sixteen-sided cone. Cone counts are 3n+4=52 vertices and 2n=32 triangles. It follows an illustrative eccentric ellipse and points its tail along normalize(pcomet−pSun), not opposite its velocity. Rotating +Y onto that direction and shifting by half the tail length keeps the wide end on the nucleus. The real heliosphere is asymmetric; these boundaries and this comet are explanatory simplifications.

HUD glyphs are screen-space quads with two triangles each. World labels are projected into the view. Transparent rings/glow are drawn after opaque surfaces, with depth testing and no depth writes. Sources: `EnvironmentBuilder`, `RingGenerator`, `StarfieldGenerator`, `Comet`, `InstancedField`, `TextRenderer`. Report §3.2.

# Slide 6: Transforms, hierarchy, scale and cameras

For column vectors, pclip=P V Mworld plocal. Local M=T R S applies scale first, then rotation, then translation. A child's world matrix is Mparent Mlocal. Changing order changes the result: scaling a translated object is not the same as translating a scaled object.

Worked example: local (1,0,0), scale two, rotate +90° about Y, then translate (10,0,0) gives (10,0,−2). A parent translation (0,3,0) gives (10,3,−2). The normal transforms by normalize((M3×3 inverse) transpose × n), preserving perpendicularity after nonuniform scaling.

The planet frame contains orbit translation, tilt and radius. The planet's own rendered sphere receives an additional spin; moons and rings use the unspun parent. Moon local scale and orbit distance divide by parent radius to avoid applying that scale twice. Voyager parts inherit one root quaternion and translation.

Display laws: body radius=.50(Rkm/6371)^.60, Sun capped at six; heliocentric distance=12(AU/.387098)^.55; moon orbit=Rparent(1.6+sqrt(aKm/RparentKm)); spacecraft size=metres×.006. These preserve order and make the scene inspectable, while the HUD reports physical data. They are presentation laws, not literal scale.

Floating origin subtracts the eye from world translations in double precision before float upload. Lights, analytic spheres and spacecraft rays use the same frame. The view matrix assumes the eye at zero. Log depth log2(1+w)/log2(1+far) addresses depth-buffer distribution, separately from position precision.

Four camera cases: FreeFly uses independent basis motion; Focus orbits a body; Chase rides in Voyager's frame or a flyby-facing frame; Inspect orbits a hardware target. Orbit offset=(r sinyaw cospitch,r sinpitch,−r cosyaw cospitch), rotated by the rig frame. Smooth transitions prevent jumps. Picking builds the camera ray from pixel/NDC and compares angular object bounds, with tolerance for tiny targets.

Sources: `Transform`, `SceneObject`, `CelestialBody`, `ScaleManager`, `Camera`, `CameraController`, `Renderer`. Report §3.3.

# Slide 7: Every motion has an owner and a clock

Historical motion uses one Julian date. Horizons rows supply position and velocity. With s=(JD−JD0)/h, Hermite interpolation is p(s)=h00 p0+h10 h v0+h01 p1+h11 h v1, where h00=2s³−3s²+1, h10=s³−2s²+s, h01=−2s³+3s², h11=s³−s². It matches both endpoint position and derivative. Differentiating and dividing by h gives velocity. Ecliptic coordinates map from (x,y,z) to scene (x,z,y), followed by radial compression.

Near an encounter, ordinary compression would place Voyager inside the enlarged planet. With rendered offset g and clearance c, L=|g|+(sqrt(|g|²+c²)−|g|)(1−smoothstep(4c,8c,|g|)). Place the craft at planet+normalize(g)L. The correction preserves direction and fades by 8c. It is a display correction. Planet-centered spacecraft vectors near encounters are composed with the same-date heliocentric planet positions.

Moons use r=a(1−e²)/(1+e cosν), p=r(cosν,0,sinν). Their visual rate is νdot=2π×.4/Pdays; spin is ψdot=2π/Phours. Signed periods create retrograde cases. Moon true anomaly advances at a constant rate, so its timing is not exact Kepler motion and its phase is not date-synchronized. Surface spin does not drag children. The screenshot strips show actual successive states.

Manual flight uses q'=normalize(q qyaw qpitch qroll); multiplying on the right turns about the ship's axes. Yaw/pitch rates are 1.1 rad/s and roll is 1.6. Thrust changes velocity by ±forward×1.5Δt, with an eightfold boost; vertical thrust uses the ship's up axis. Position integrates velocity. Braking subtracts min(speed,2accelΔt), so it cannot overshoot and reverse. Speed caps at twelve render units/s. Turning changes orientation without automatically redirecting inertial velocity.

The comet aligns its cone with pcomet−pSun, handling parallel/opposite axis cases. Cameras and manual flight use real time even when P pauses planets/moons/spin. The fields and scan platform remain static. After 2030, planets use two-body Kepler continuation and Voyager ballistic continuation, not high-accuracy indefinite mission data. The clock is finite.

Sources: `Trajectory`, `MissionEphemeris`, `SimulationClock`, `CelestialBody`, `Voyager2`, `Comet`. Report §3.4.

# Slide 8: Three light types—and a moving headlamp

Use source geometry first. At surface p, n is the unit normal, v points to the eye and l points toward the light. A point source uses l=(pL−p)/distance. A directional source uses normalize(−direction), the same direction everywhere. A spotlight adds a cone to a point source. Phong is a surface-response model, not a light type.

Lambert diffuse is max(n·l,0). Attenuation is 1/max(kc+kl d+kq d²,10^-4). The Sun defaults to coefficients (1,0,0), keeping outer planets visible. F7 selects (.3,0,.7/400), giving one at distance twenty and about .03 at Neptune. This is a deliberately compressed-distance falloff.

The headlamp position is the eye and its axis is camera.forward. Its intensity is 1.4, with warm colour and quadratic reach equal to three times nearest-surface distance. θ=(-l)·direction. Cone factor C=clamp((θ−cos18°)/(cos12°−cos18°),0,1). It is full within 12°, zero outside 18°, and smoothly interpolated in cosine between them. Moving the eye changes both position and direction; the strip is actual runtime evidence of the moving source.

The ordinary directional fill is cool, intensity .35, with normalized direction (.25,−1,.15). Inspect adds a .45 studio fill aligned with the view. The comparison images show Sun only, Sun+headlamp and Sun+directional fill; the Sun is enabled throughout. They are not falsely presented as three isolated scenes.

Colour is albedo×(ambient+VS×SunDiffuse+otherDiffuse)+VS×SunSpecular+otherSpecular, ambient=.07. Only Sun terms receive Sun visibility. Auxiliary lights can illuminate a night side without changing the eclipse calculation; they do not cast shadows. F5/F6/F7 toggle the examples; K disables lighting.

The visible equation panel on this slide identifies ambient $k_a=0.07$, Lambert diffuse $D=\max(n\cdot l,0)$, Phong specular $S_P=k_s\max(r\cdot v,0)^p$, and the implementation's full light composition. The matching report derivation is Chapter III, Section 3.5, PDF pages 12–13. Slide 9 places the Phong and Blinn–Phong specular equations below their matched images.

Sources: `LightingController.cpp`, `LightingUniforms.cpp`, `shaders/lighting.glsl`. Report §3.5.

# Slide 9: Five shading techniques, matched views

These are paused views with the same geometry, texture and lighting. Flat obtains a face normal from normalize(dFdx(p)×dFdy(p)), revealing facets. Gouraud computes Phong light terms at vertices in scene.vert and interpolates them across a triangle. A narrow highlight located between vertices can disappear; normal maps are disabled in Gouraud. Phong shading interpolates normals, renormalizes and evaluates a reflect-vector highlight per fragment. Blinn–Phong uses a half-vector per fragment and is the default.

For n, l and v: r=reflect(−l,n)=2(n·l)n−l. Phong specular is ks max(v·r,0)^power. Blinn uses h=normalize(l+v), with ks max(n·h,0)^(2power). The doubled exponent is this implementation's approximate highlight-width match, not an identity between the two models. Specular is evaluated only where Lambert is positive.

Toon maps positive diffuse to bands: above .75 gives 1, above .40 gives .62, above .12 gives .30, otherwise .06. A nonpositive Lambert term stays zero. Specular is thresholded at .5 and rim ink is added where n·v<.22. The pictures should illustrate those decisions rather than claiming every material must show a large Phong/Blinn difference.

Separate material behaviors include Unlit for Sun/guides, LitTwoSided for rings, and Glow for Sun/comet shells. Two-sided lighting flips the normal toward the viewer and uses |n·l|. Glow alpha=|n·v|^power×opacity and additive blending; translucent draws are depth-tested without depth writes. These are not additional F3 techniques.

Sources: `scene.vert`, `scene.frag`, `lighting.glsl`. Report §3.6. The Earth and spacecraft rows demonstrate both broad curved surfaces and authored hardware under all five cases.

# Slide 10: Visibility: eclipses, ring shadows and self-shadow

Shadows determine whether light reaches a point. Diffuse/specular shading determines how it responds to light that reaches it. Only the Sun casts shadows. F4 cycles Off, Hard and Soft in the matched Saturn screenshots. Point to the planet's shadow on the rings and the ring shadow on the planet.

Hard mode tests one ray toward the Sun center. Sphere intersections use the quadratic and rings use a plane intersection followed by an inner/outer-radius test. Soft mode treats the Sun as a finite disc with angular radius asin(.6/dSun). Occluding spheres project discs with their own angular radii and separation. Their circle-overlap fraction gives analytic penumbra coverage: visibility=1−overlapArea/(π solarAngularRadius²). There is no random Monte Carlo sampling. Translucent rings multiply visibility by 1−opacity.

Self-hit prevention skips the source sphere, offsets ring origins by 10^-4 times outer radius, and uses biased triangle-ray origins. An uncorrected origin would hit its own surface and create dark acne.

Voyager's raster self-shadow uses a 2048² Sun depth map fitted to the craft's bounds. Transform the fragment into the Sun projection and compare its depth with the nearest stored depth. Nine 3×3 filtered lookups average visibility (PCF). Polygon offset/bias mitigates self-shadow artifacts. This visibility multiplies the analytic body/ring Sun visibility. The shadow map covers the spacecraft, not planet receivers.

The older per-fragment BVH self-shadow path was expensive in close-ups. The shadow map moves that work into a selective light pass. Earlier optimization measurements are documented separately; do not present them as a fresh ablation. Headlamp and fill remain unshadowed.

Sources: `Renderer::renderShadowMap`, `ShadowMap`, `scene.frag`, `raytrace.glsl`. Report §3.8 and learning guide Chapters 7 and 11.

# Slide 11: Ray tracing: nearest hit, light, reflection

A primary ray comes from the same camera basis as projection: normalize(forward+ndcX tan(fov/2) aspect right+ndcY tan(fov/2) up). Ray position is o+td. For a sphere, substitute into |o+td−c|²=R². With unit d, b=(o−c)·d and f=|o−c|²−R²; roots are −b±sqrt(b²−f). A negative discriminant misses; accept the nearest positive root. A ring uses t=((c−o)·N)/(d·N), rejects parallel/backward hits and checks inner≤|hit−center|≤outer.

Triangle intersection solves o+td=A+u(B−A)+v(C−A). Möller–Trumbore uses E1=B−A, E2=C−A, P=d×E2, det=E1·P, T=o−A, u=(T·P)/det, Q=T×E1, v=(d·Q)/det and t=(E2·Q)/det. Accept positive t, u≥0, v≥0 and u+v≤1. Interpolate UV and normal with weights (1−u−v,u,v), then normalize the normal.

Voyager's BVH bounds 5,828 triangles with 6,723 nodes, depth 24, leaf size at most four and a twelve-bin surface-area heuristic. The split estimates child surface-area probabilities times their triangle costs. A slab test intersects ray intervals along three axes. A 32-entry stack visits the near child first; nearest-hit distance prunes farther boxes and shadow rays stop early. Nodes occupy two RGBA32F texels and triangles seven; inverse rotation and relative translation move each ray into the craft frame. The geometry is not rebuilt every frame.

F9 traces the Sun, twenty-five other bodies, fifteen ring bands and Voyager. Hit material, UV and normal feed Blinn–Phong lighting and Sun shadow rays. Translucent rings continue up to four layers. F10 adds one mirror ray with direction d−2(d·n)n and material reflectivity. Secondary reflected rays omit Voyager. The environment—stars, guides, belts and comet—remains rasterized and shares logarithmic depth. The tracer resamples albedo into 26 layers of 1024×512; it does not use raster normal/specular maps or F3 shading alternatives. There is no refraction or global illumination.

Sources: `raytrace.frag`, `raytrace_mesh.glsl`, `TriangleBvh`, `RayTracer`. Report §3.8. Distinguish primary, shadow and reflection rays; the BVH changes search cost rather than the triangle definition.

# Slide 12: Textures: colour, atlas, relief and specular mask

Albedo RGB supplies surface colour and multiplies material tint before lighting. UV maps reuse one sphere for 26 bodies. Seam duplication, reversed longitude and vertically flipped image loading preserve geographic orientation. Filtering and mipmaps control distant minification. An albedo image does not alter vertices or the silhouette.

Voyager's NASA hardware atlas provides eleven textured finishes; copper is untextured. uvTransform stores rectangle offset xy and scale zw, so uv'=uv×zw+xy. Materials select regions for foil, white paint, metals, blankets, louvres and lenses. NASA geometry is not imported. Texture-based louvres and wrinkles reduce geometric cost, while the dish's actual curvature remains modeled.

The derived normal map uses luminance H=.2126R+.7152G+.0722B in normalized RGB. Sobel 3×3 gradients dx/dy produce normalize(−strength dx,−strength dy,1), encoded as .5n+.5. Longitude wraps, latitude clamps. Position/UV screen derivatives reconstruct the tangent/bitangent frame without a tangent vertex attribute. Colour-derived relief is approximate: it is not measured elevation and does not displace the mesh. The explanatory channel plate resizes albedo before derivation; the runtime comparison itself is unmodified.

Earth's specular mask marks water when B>1.25R, B>1.02G and R+G+B<1.5, assigning 255 to water and 30 to land. The mask scales specular strength. F8 disables both maps. Gouraud does not use fragment normal maps; the traced path uses albedo but not the raster-derived maps. Rings use band colour/opacity and guides use tint rather than nonexistent texture maps.

Credits are in the asset manifests: Solar System Scope CC BY 4.0 maps and NASA resource provenance. Sources: `SurfaceMaps`, `Texture2D`, `MaterialLibrary`, `VoyagerModelBuilder`, `scene.frag`. Report §3.7.

# Slide 13: Thank you

Close with the chain of ideas: indexed geometry creates objects, transforms create relationships and pose, motion updates state, and illumination/shading/textures/rays create the visible result. Invite a question about any equation and connect it to the corresponding screenshot and source file.

Useful defense questions include seam duplication; pole triangle omission; hard-edge box normals; the dish derivative; child transforms without inherited spin; dated Hermite versus visual moon motion; body-axis quaternion multiplication; spotlight cosine falloff; Gouraud versus Phong; albedo versus normal/specular maps; ray-sphere roots; triangle barycentric limits; and BVH pruning.

Debug/Release builds, repository checks, runtime tours and the brief-input regression supply evidence. The eight-view Release benchmark on the observed Radeon RX 590 at 1440×900 measured .64–1.15 ms for raster views and 6.60 ms for traced Voyager, mean 1.71 ms. This is one machine/run. GPU timing includes HUD; draw counters omit shadow-depth, fullscreen tracing and HUD; GPU query retrieval can wait.

State limitations accurately: display scale and flyby clearance are compressed; moon phases are visual; belts and scan platform are static; hardware and colour-derived relief are approximate; only the Sun casts shadows; tracing is hybrid with one mirror bounce and no Voyager in secondary reflections; after-data predictions are approximate and the clock is finite.

The report, speaker notes, fundamentals guide, object handbook and provenance files support questions. The final video is reserved for the author to record and attach after the two introduction slides.
