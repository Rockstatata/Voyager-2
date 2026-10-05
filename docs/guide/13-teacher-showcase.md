# Teacher showcase: evidence, explanation and live changes

Start here for the new teacher instructions. Use the [rehearsal workbook](12-presentation-rehearsal.md) for the longer derivations and [change recipes](10-how-to-change-things.md) for editing. This document is a practical speaking and demonstration plan, checked against the current code. It does not replace your report or submitted proposal.

## Important features to point out during the video

Slide 2 now introduces the important features beside three actual screenshots. Its speaker notes provide the narration; the ten technical slides show their details. Use this map to explain why each feature matters and how to make it visible.

| Important feature | What makes it useful in this project | Screenshot / slide | What to show and say |
| --- | --- | --- | --- |
| Dated mission and coherent flybys | Planets and Voyager use the same NASA/JPL date; interpolation produces smooth flight | 2 and 7 | Bookmarks 1–6 and running date: explain the shared clock and Hermite positions |
| Authored, inspectable spacecraft | Reusable primitives create 76 assemblies, curved dish and 18 hardware targets | 2, 4 and 6 | I, then ./,: name a component and explain its vertices, normals and parent transform |
| Four cameras and manual six-degree flight | Users choose bodies/hardware and change viewpoint, spacecraft thrust and pose | 6–7 | Select a world, orbit/zoom, then V and thrust/yaw/brake; explain input → update → render |
| Precision across enormous scales | Floating origin keeps hardware close-ups stable beside interplanetary positions | 6 | Explain double-precision eye subtraction before float matrices; use overview and Inspect views |
| Moving spotlight and five shading modes | The same scene demonstrates source geometry and per-vertex/per-fragment surface response | 8–9 | Freeze the view; F5/F6 and F3; explain the visible diffuse/specular change and calculation stage |
| Shadows and bonus hybrid ray tracing | Analytic body/ring visibility, spacecraft depth-map self-shadow, triangle BVH and one reflection | 10–11 | F4 then F9/F10; point to ring/eclipse shadows and explain nearest hit and reflected direction |
| NASA textures and derived surface detail | Atlas finishes, inferred relief and water-mask highlights add detail without extra geometry | 12 | F8 off/on; explain albedo versus normal/specular maps and unchanged silhouette |
| Efficient, complete environment | Four ring systems, 8,500 instanced rocks, stars and a dynamic comet expand the scene | 3 and 5 | Point to rings, rock fields and comet; explain shared meshes and one draw per rock field |

Describe these as strengths of the project's implementation and integration. The standard graphics algorithms have their usual sources; there is no claim that Gouraud, Phong, BVHs or ray tracing were invented here.

## 1. What is ready, and what is still your responsibility?

| Teacher requirement | Finding | Evidence / demonstration |
| --- | --- | --- |
| Lighting | Implemented: point Sun, camera spotlight and directional fill | `LightingController::build`, `lighting.glsl::evaluateLights`; report 3.5; F5/F6/F7 |
| Transformation | Implemented: translation, quaternion rotation, scale, parent transforms, view/projection and normal matrix | `Transform`, `SceneObject::worldMatrix`, `scene.vert`; report 3.3; orbit a body / inspect hardware |
| Gouraud shading | Implemented: lighting at vertices, light terms interpolated into fragments | `scene.vert` Gouraud branch, `scene.frag` Gouraud branch; report 3.6; F3 until HUD says Gouraud |
| Phong shading | Implemented: interpolated normal normalized and reflection-vector lighting evaluated per fragment | `scene.frag`, `lighting.glsl`; report 3.6; F3 until HUD says Phong |
| Motion / animation | Implemented: dated spacecraft/planet playback, moon revolution, axial spin, manual flight and comet orientation | `SimulationClock`, `Trajectory`, `MissionController`, `Voyager2`; report 3.4 |
| Interaction | Implemented: input changes cameras, object selection, simulation speed, visibility, lights and rendering | `Input`, `Application::handleKeys`, camera/light controllers; F1/F2/P/Tab/I/F3/F9 |
| Bonus ray tracing | Implemented: analytic sphere/ring visibility and a hybrid Whitted view with spacecraft triangle BVH | `raytrace*.glsl`, `RayTracer`, `TriangleBvh`; report 3.8; F9/F10 |
| Introduction, unique screenshots and thanks | Present in both current PPTX files, thirteen slides each | Slides 1–2 introduction, 3–12 technical screenshots, 13 thanks |
| Demo video | Cue exists; neither PPTX contains embedded video media | Attach/play your own final video after slide 2; test playback on the actual showcase machine |
| Detailed report with equations and credits | Present: nineteen-page report, eleven-page methodology, seven external references | [Report](../report/Voyager_2_Graphics_Report.pdf), sections 3.1–3.9 and bibliography |
| Submitted proposal versus final implementation | Audited against the two-page proposal: one promised behavior is partial | Progressive travelled-path drawing is absent; optional gravity-assist markers are absent; see the explicit comparison below |
| Explain code / change parameters | Code and recipes are available; personal fluency requires rehearsal | Complete the live-change drills below and answer without reading |

The current Debug build and both repository verifiers pass. The fresh twenty-image shading tour checks actual application output. These automated checks do not certify your speaking preparation, final video playback, or every mouse/flight action on a different machine. See the [recorded audit](../report/validation/showcase-audit.json).

### Proposal completion: the original two-page proposal

Source: [submitted proposal](../report/Voyager_2_Project_Proposal_2_Page.pdf), sections 1–10. Do not invent a completion percentage or count optional additions as substitutes for promised behavior. The proposal explicitly permits visual scales and excludes full gravity simulation, so those are within its scope.

| Submitted promise | Status | Function or asset | Screenshot / control | Qualification |
| --- | --- | --- | --- | --- |
| Earth → Jupiter → Saturn → Uranus → Neptune → interstellar journey; Sun and major planets | Complete | `MissionController`, `MissionEphemeris`, body catalogue | Bookmarks 1–6; running date | Uses offline Horizons data and display scales |
| Primitive spacecraft with dish, boxes and cylinders; hierarchical parts | Complete | `VoyagerModelBuilder`, `SceneObject::worldMatrix` | I, then component selection | Geometry authored procedurally, NASA imagery supplies texture |
| Visible 3D trajectory, starfield and heliopause boundary | Complete | `MissionController::buildTrajectory`, `EnvironmentBuilder` | T; overview / heliopause bookmark | Path is stored data, boundary is a visual model |
| Trajectory drawn progressively as the spacecraft travels (sections 3 and 9) | **Partial** | `buildTrajectory` uploads the entire line strip | T displays whole path | No date-dependent reveal/count; do not claim progressive drawing |
| Follow path heading, slow planet rotation, curved gravity-assist flybys | Complete within proposed visual scope | `placeVoyager`, body spin, Horizons curves | Playback and flyby bookmarks | Not a real-time gravitational solver, as permitted by proposal |
| Translation, rotation, scaling, composite and hierarchical transforms | Complete | `Transform`, scene graph, camera and shaders | Body/spacecraft views | Column-vector TRS and parent composition |
| Sun, ambient/diffuse/specular, Flat/Gouraud/Phong and correct transformed normals | Complete | Light controller; scene/lighting shaders | K, F3, frozen comparisons | Extra Blinn/Toon, spot/fill beyond original scope |
| Overview, follow, flyby and free cameras | Complete | `CameraController` | H, C, bookmarks, RMB | Flyby framing is encounter-aware Chase, not a separately named mode |
| Pause, restart, speed, camera switching, trajectory toggle and shading selection | Complete | `Application::handleKeys` | P, 1, =/-, C, T, F3 | 1 resets to launch and resumes; Backspace only resets speed/unpauses |
| Mission labels, timeline and encounter information | Complete as HUD/date/bookmark presentation | `HudOverlay`, mission bookmarks | F2, L, bookmarks 1–6 | No draggable timeline widget was promised or implemented |
| C++/OpenGL/GLFW/GLAD/GLM; update before render; materials/depth/transparency | Complete | Core loop, Renderer, materials, shaders | Running program / heliopause | Input precedes update; transparent passes follow opaque |
| Optional gravity-assist path markers | Absent, explicitly optional | Encounter bookmarks exist; no dedicated marker objects | Use bookmarks for encounter discussion | Distinguish optional markers from curved path, which exists |

The report's opening names the proposed feature groups, their implemented counterparts and additional features. Extra implementation includes the detailed inspection system, additional bodies/rings, rock fields, comet, surface maps, extra shading/light types, analytic shadows and hybrid BVH ray tracing. The detailed comparison above is preparation material for answering scope questions, separate from the report.

**Speaking pattern:** “The proposal promised X. I implemented it in Y. Here is the result. The limitation is Z.” For an additional feature: “This was added beyond the proposed scope.”

## 2. Prepare the showcase machine

1. Open your final edited presentation and the report PDF. Keep the original proposal and this guide available separately.
2. Open the final video in advance. Check sound if used, offline playback, readable labels and the exact point where each technique changes. The existing `Voyager_2_Demo.mp4` is an earlier recording; use it only if you have checked that it matches your final explanation.
3. Run from the repository root using `.\scripts\run.ps1 -NoBuild`; assets/shaders use relative paths. Keep the console available for shader messages and selected technique names.
4. Rehearse F1/F2, selection, pause, F3 comparisons, Inspect and F9 on this machine. Keep a raster fallback and the existing comparison screenshots ready.
5. Open `LightingController.cpp`, `MaterialLibrary.cpp`, `lighting.glsl`, `scene.vert`, `scene.frag`, `Transform.h` and `Voyager2.cpp` in editor tabs. Know which changes require compilation and which only require restarting.
6. Save any source edit before rebuilding; stop the running program first so Windows does not lock the executable. Restore temporary demonstration values after showing their effects.

## 3. Opening: say what the project does

Use this model in your own words:

> My project is an interactive Voyager 2 solar-system explorer in C++ and OpenGL. I construct planets and spacecraft geometry, place them with hierarchical transforms, and replay dated mission flybys. The user can select bodies, inspect hardware, fly manually, pause or speed up motion, and compare lighting, shading, textures and ray tracing. My focus is making graphics techniques visible and explainable in one application.

বাংলায় বললে: “এখানে শুধু একটা solar system দেখাচ্ছি না; একই scene ব্যবহার করে geometry, transformation, motion, lighting, shading আর interaction কীভাবে implement করেছি সেটা দেখাচ্ছি। User input দিলে program-এর state বদলায়, update-এ নতুন অবস্থান হিসাব হয়, তারপর render-এ তার ফল দেখা যায়।”

Then give the proposal comparison using the actual submitted wording. Do not start by reciting all library names. Explain the central problem: spacecraft hardware is metre-scale while planets and trajectories span huge distances, so display scales are compressed and world positions stay double precision until the camera origin is subtracted.

## 4. Two-minute video narration

Align this script to your final recording. Timestamps are a suggested edit/narration plan, not a claim about the existing video. For every segment: name the feature, describe the computation, then identify the visible evidence. Pause playback briefly for questions rather than talking over a scene change.

| Time | Show | What to explain aloud |
| --- | --- | --- |
| 0–15 s | Labelled overview and one close planet | “26 textured bodies share indexed sphere geometry. Positions and normals define shape; UVs select surface colour. Display scales make the system explorable.” |
| 15–30 s | Jupiter/Saturn/Neptune mission views, running date | “Voyager and planets use the same Julian Date. Offline NASA/JPL position and velocity samples are interpolated with Hermite curves, so their flyby positions stay coherent.” |
| 30–45 s | Body focus, mouse orbit, pause/resume, moon/spin | “Camera input is live even when simulation time is paused. Revolution moves the centre; axial rotation changes the surface pose. Parent transforms organize these relationships.” |
| 45–60 s | Voyager Inspect; dish, truss and bus | “I built this mesh from boxes, cylinders, rods and a sampled paraboloid. The dish is curved because vertex height depends quadratically on radial distance. Inspect targets frame the authored components.” |
| 60–80 s | Fixed-view Sun/spot/fill, then Gouraud/Phong | “Diffuse intensity depends on normal–light alignment; specular intensity depends on the viewer. Gouraud computes light at vertices; Phong computes it per fragment. I keep the camera and scene fixed to compare them.” |
| 80–100 s | Shadows off/hard/soft; F9; F10 | “Shadow rays test visibility to the Sun. Voyager raster self-shadows use a depth map. The traced view finds sphere, ring and BVH triangle hits; one optional reflection follows the reflected ray.” |
| 100–115 s | Maps off/on; manual yaw and thrust | “Normal maps change the lighting normal, not the silhouette. Manual controls rotate a quaternion and integrate velocity. Turning alone does not erase existing velocity.” |
| 115–120 s | Overview / controls | “These are user-controlled rendering and simulation features. I can show the implementing functions and change their parameters.” |

Do not claim everything in the scene is dynamically simulated: the rock fields are static instanced placements. Do not claim full path tracing: the traced renderer is hybrid with bounded reflections.

## 5. Live demonstration with controlled comparisons

Use the HUD to confirm state; most controls toggle, so pressing a fixed sequence blindly can produce the wrong comparison. Restarting gives a reliable baseline. Keep F9 off while demonstrating raster Gouraud/Phong because the traced view uses its own Blinn–Phong lighting.

1. **Interaction:** show F1 help, hide it with F1, toggle the HUD with F2, select a body with Tab or a click, and orbit using RMB. Explain `keyPressed` versus a held `keyDown`. A visible overlay toggle already satisfies the teacher's example of opening/closing something through input; flight, selection and mode switching provide further interaction.
2. **Motion/transforms:** select Earth or Jupiter, pause with P after observing movement, then move the camera while paused. Resume and change speed with `=`/`-`. Explain simulation time versus real frame time. Bookmarks `1`–`6` jump dates and enter mission views; they are not six animation algorithms.
3. **Lighting:** choose a stable body view and pause. Ensure lighting is on (K), tracing is off, and observe the HUD states. Toggle F5 spotlight and F6 fill separately. Move the camera with spotlight enabled to show that its position/direction follow the eye. Inspect automatically adds a studio fill, so use a body for a clean three-light comparison.
4. **Gouraud/Phong:** remain paused, maps off for the clean comparison, and cycle F3 until each named mode appears. Keep camera, material and lights fixed. State that a dense sphere can make the difference subtle; point to the existing matched Earth/spacecraft screenshots instead of claiming a dramatic difference is guaranteed.
5. **Unique geometry:** I enters Inspect; `.`/`,` move through targets. Show the curved dish and a truss. Describe how vertices and indices are authored before discussing texture.
6. **Shadows/tracing:** choose Saturn with a visible ring/body shadow. F4 cycles off/hard/soft; confirm status. F9 switches the primary render path. F10 changes the optional reflection only while examining the traced view. Explain the static Voyager triangle BVH and analytic planet/ring tests.
7. **Manual flight:** V enters Manual and Chase. Tap W to thrust, A/D to yaw, R/F to pitch, Q/E to roll, X to brake. Do not press W while in Inspect expecting spacecraft thrust. V returns to Historical; its position resets to the dated trajectory.
8. **Closing:** return to the overview with H. Esc first closes help or exits Inspect; otherwise two presses within two seconds quit. Save quitting until the end.

## 6. The equations you should be able to reconstruct

Let $a$ be albedo times material tint; $n,l,v$ are unit normal, direction to light and direction to viewer. Colour products below are componentwise. Light colour already includes intensity when uploaded. This renderer groups the Sun separately so its visibility factor does not darken other lights or ambient.

$$D=\max(n\cdot l,0),\qquad r=2(n\cdot l)n-l,\qquad S=k_s\max(r\cdot v,0)^p.$$

Specular is only evaluated on a lit face with positive specular strength. Ambient is $a\,k_a$, with $k_a=0.07$ by default. Each point/spot light has attenuation

$$A(d)=\frac{1}{k_c+k_l d+k_qd^2}.$$

For the spotlight, with travel direction $s$, $\theta=(-l)\cdot s$:

$$C=\operatorname{clamp}\left(\frac{\theta-\cos18^\circ}{\cos12^\circ-\cos18^\circ},0,1\right).$$

The Sun's diffuse/specular terms include its attenuation and colour. With $V_S$ Sun visibility, the actual composition is

$$I=a(k_a+V_SD_S+D_{other})+V_SS_S+S_{other}.$$

**Explain each term:** ambient keeps unlit surfaces readable; diffuse is angular surface response independent of the viewer; specular is a view-dependent highlight. A point light has a position and distance attenuation; a directional light supplies one direction; a spotlight adds the angular cone weight. Ambient is an approximation, not computed indirect global illumination.

**Numerical check:** $n=(0,0,1)$ and $l=(0,0.6,0.8)$ give $D=0.8$. If $r\cdot v=0.9$, $k_s=0.5$ and $p=8$, then $S=0.5(0.9)^8\approx0.2152$. A white unshadowed unit light with $a=0.5$ gives $I=0.5(0.07+0.8)+0.2152=0.6502$. At visibility zero only the Sun terms disappear, leaving ambient 0.035 when other lights are absent.

**Gouraud versus Phong:** Gouraud computes light terms at the triangle's vertices and interpolates them over covered fragments. Phong interpolates normals/positions and evaluates lighting per fragment. A highlight located between vertices can be missed by Gouraud. Phong shading (evaluation/interpolation method) and Phong reflection (the specular equation) are different concepts; this project uses Phong reflection in both its Gouraud and Phong modes.

**Blinn–Phong:** $h=\operatorname{normalize}(l+v)$, $S=k_s\max(n\cdot h,0)^{2p}$. The factor two is this project's approximate highlight-size match, not a universal conversion.

**Transforms:** $p_{clip}=PV M_{parent}TRS p_{local}$. With column vectors, scale acts first, rotation next and translation last. Normals use $\operatorname{normalize}((M^{-1})^T n)$ for the effective linear transform. Work through $p=(1,0,0)$, uniform scale 2, 90-degree rotation about Z, translation $(3,1,0)$: the result is $(3,3,0)$. Translation is not added to normals.

**Animation:** $x_{new}=x+v\Delta t$, $v_{new}=v+a\Delta t$ illustrate time-based integration. The spacecraft implementation updates velocity and pose with the real timestep; mission playback advances one shared astronomical date. Hermite uses endpoint positions and velocities:

$$p(u)=(2u^3-3u^2+1)p_0+(u^3-2u^2+u)\Delta t v_0+(-2u^3+3u^2)p_1+(u^3-u^2)\Delta t v_1.$$

Here $u$ is normalized interval time and $\Delta t$ is sample spacing in days for velocities in AU/day. Explain why multiplying the velocities by the interval fixes units. Moon/spin animation uses presentation rates; do not call every animation an exact NASA state.

**Geometry:** a sphere samples $\phi=\pi i/32$, $\theta=2\pi j/64$ at $p=(\sin\phi\cos\theta,\cos\phi,\sin\phi\sin\theta)$. The grid has $(32+1)(64+1)=2145$ vertices, seam duplication and $2(64)(32-1)=3968$ triangles after pole degenerates are omitted. A box needs distinct face normals/UVs, so this builder uses 24 vertices, not just its eight geometric corners.

**Ray tracing:** $p(t)=o+td$. Substitute into $\|p-c\|^2=R^2$ to get $At^2+Bt+C=0$, with $A=d\cdot d$, $B=2d\cdot(o-c)$, $C=\|o-c\|^2-R^2$. Choose the nearest valid positive root. Shadow rays test whether an occluder lies before the light; reflection uses $d_r=d-2(d\cdot n)n$. The BVH rejects bounding boxes before testing leaf triangles. It accelerates tests; it does not replace triangle intersections.

For diagram-based explanation, open the [full visual catalogue](../report/Visual_Fundamentals.md), and [lighting](05-lighting.md), [shading](06-shading-techniques.md), [tracing](07-ray-tracing.md) chapters. The catalogue indexes the exact image filenames, including the indexed sphere cell.

## 7. Code navigation: explain responsibilities, not lines

| Teacher question | Open | Explain the data flow |
| --- | --- | --- |
| Where does a frame start? | `Main.cpp`, `Application::run/update/render` | Construct application; input changes state, update advances it, render draws it |
| Where is interaction handled? | `Input`, `Application::handleKeys`, `LightingController::handleKeys` | Poll GLFW, distinguish press/hold, change simulation/camera/light state |
| How are triangles made? | `UvSphereGenerator::generate`, `ParabolicDishGenerator::generate`, `Mesh` | CPU vertex/index arrays, winding/normals/UVs, GPU upload and indexed draw |
| Where does spacecraft shape come from? | `VoyagerModelBuilder::build` | Place primitive assemblies, select atlas finishes, register component anchors |
| Where are transforms applied? | `Transform.h`, `SceneObject::worldMatrix`, `Renderer`, `scene.vert` | Local TRS, parent composition, camera-relative conversion, view/projection |
| Where are Gouraud and Phong different? | `scene.vert`, `scene.frag`, `lighting.glsl::evaluateLights` | Stage of lighting evaluation; fragment-normal path; reflect-vector term |
| Where are the lights configured? | `LightingController::build`, `LightingUniforms` | CPU light type/pose/cone values are uploaded to shader uniforms |
| Where is motion calculated? | `SimulationClock::update`, `Trajectory.cpp`, `MissionController::update`, `Voyager2::applyManualControl` | Shared date and interpolation versus real-time manual control |
| Where does clicking select objects? | `CameraController::pickAt` | Pixel becomes view ray; angular tolerance helps select small worlds; choose target and transition camera |
| Where is tracing accelerated? | `TriangleBvh`, `RayTracer`, `raytrace_mesh.glsl` | Build static local-space hierarchy; upload to texture buffers; bounding-box/triangle traversal |
| Where does surface detail come from? | `MaterialLibrary::surface`, `SurfaceMaps`, `scene.frag` | Decode albedo, derive maps, sample them and perturb normals without changing geometry |

Follow one vertex through the pipeline: generator → position/normal/UV in `MeshData` → VBO/EBO upload with VAO layout → vertex transform → rasterized/interpolated fragment → texture and lighting → depth/blending → image. VAO describes buffer interpretation; it is not a second mesh buffer.

## 8. Parameter-change drills for the teacher

Before touching a value, say what it controls and predict the effect. Change one value, save, rebuild/restart as needed, reproduce the same camera/light/shading state, show the difference, then restore it. Do not improvise unrelated edits.

| Drill | Exact edit | Predicted visible effect | How to isolate / apply |
| --- | --- | --- | --- |
| Brighter ambient | `LightingController::build`: `state.ambient = 0.07f` → `0.20f` | Dark-side surfaces become brighter; geometry and direct-light direction stay the same | Fixed paused body, K on, fill/headlamp off; C++ rebuild and restart |
| Narrower spotlight | Same function: 12°/18° → 6°/10° in `cos(radians(...))` | Smaller illuminated cone; smooth edge between inner/outer bounds | F5 on, keep pose fixed; inner angle must remain less than outer; C++ rebuild |
| Wider highlight | `MaterialLibrary::surface`, ocean preset: `specularPower = 48` → `12` | Broader highlight at the same strength | Earth, Phong mode, fixed view, maps off to isolate the material response; C++ rebuild |
| Remove highlight | Same preset: `specularStrength = 0.55` → `0` | Specular glint disappears; diffuse remains | Same comparison; C++ rebuild |
| Faster historical playback | `SimulationClock.h`: `kBaseDaysPerSecond = 120` → `240` | Twice the base date advance at the same speed factor/encounter state | Show cruise or N disables slowdown; C++ rebuild; `=` provides a runtime alternative |
| Change Toon boundary | `lighting.glsl::toonBand`: first threshold `0.75` → `0.90` | Smaller region in the brightest diffuse band | Toon mode, frozen pose/light; shader save and restart, no C++ rebuild |

Example spoken answer: “The exponent controls highlight width. The dot product is between zero and one, so raising it to a larger power suppresses more angles away from perfect reflection. Decreasing it should broaden the highlight. I'll change only that exponent and compare the same view.”

The preset affects every body using it; the ocean preset currently selects Earth. Know this sharing before a teacher asks why several objects changed. Shader source is loaded at startup; there is no automatic live shader reload. Changing a cone or material is reversible; do not permanently modify scene clearances just to show a transform.

## 9. Questions to answer without the slides

1. Why does Gouraud miss some highlights? Explain interpolated vertex light versus nonlinear fragment lighting.
2. Is Phong shading the same as Phong illumination? Distinguish stage/normal interpolation from reflection equation.
3. What does ambient do? It is a constant approximation; no global illumination is solved.
4. Why normalize the interpolated normal? Interpolation does not preserve vector length; lighting needs angular dot products.
5. Why inverse-transpose normals? Nonuniform scale changes the tangent plane; normals must remain perpendicular.
6. What is interaction here? Name a key, the changed state, and the visible result, then show it.
7. What keeps a planet and Voyager on the same date? One SimulationClock and dated interpolation.
8. Why can I move while P is paused? Camera/manual flight use real frame time; astronomical time is paused.
9. Why are planets not literally to scale? Readability across huge ranges; scale mapping differs from source physical units.
10. Is the spacecraft imported? Its geometry is procedurally authored; NASA imagery supplies texture.
11. Is this full ray tracing? The primary body/ring/craft path is traced; environment remains rasterized; one optional bounce, no GI/refraction, reflected rays omit Voyager.
12. Which promised features are incomplete? The trajectory exists and is toggleable, but progressive travelled-path reveal is missing. Optional gravity-assist markers are absent. The remaining promised feature groups are implemented; explain visual motion/data qualifications rather than inventing a percentage.

Read the detailed expandable answers in the [rehearsal workbook](12-presentation-rehearsal.md) after trying these from memory.

## 10. Final hour rehearsal

Spend 10 minutes on the opening and actual proposal comparison; 15 on the video narration and live controls; 15 on lighting/Gouraud/Phong equations and one numerical example; 10 on two parameter-change drills; 10 answering the questions aloud. If you cannot explain an equation symbol or predict a parameter effect, return to its learning chapter. Knowing where the code is makes the discussion manageable; memorizing each line is unnecessary.

Your minimum ready state is: final video actually plays; report is open; original proposal comparison is honest; you can show every required feature; you can explain Gouraud versus Phong, TRS and the three illumination terms; and you have successfully practiced at least two small parameter changes.
