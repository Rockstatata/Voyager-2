# CLAUDE.md

## Source of Truth

This is a Windows/OpenGL 3.3 Core solar-system explorer built with C++20, MSVC, GLAD, GLFW, and GLM. Read `docs/Voyager_2_Solar_System_Implementation_Bible.md` before architectural work. It is authoritative when guidance conflicts. Section 45 defines the phases, section 43 the manual tests, and sections 49–52 the conventions, failure modes, and canonical interfaces.

`docs/copilot-plan-phase.md` records the completed Phase 1 refactor. `docs/vscode-working.md` is historical; `.vscode/` and `scripts/` are the working configuration. `ChatGPT.cpp` is uncompiled reference material, not production code.

**Current state:** all bible phases are complete. 26 textured bodies share one 32×64 UV sphere. Planets and Voyager 2 are evaluated from dense Horizons state vectors (`MissionEphemeris`) at the single `SimulationClock` Julian Date; flybys match NASA's closest approaches. Voyager 2 is procedural (`VoyagerModelBuilder`) with Historical and 6-DOF Manual flight. Sun lighting, glow, translucent rings, floating origin, logarithmic depth, HUD/labels (`TextRenderer`, project-authored `BitmapFont`) and free-flight camera autonomy are in place. `docs/objects/README.md` indexes how each piece works.

## Build, Run, and Verify

The project requires MSVC platform toolset `v145`, the Windows SDK, and Visual Studio's **Desktop development with C++** workload. There is no CMake or automated test suite.

```powershell
.\scripts\build.ps1                          # Debug x64
.\scripts\build.ps1 -Configuration Release   # Release x64
.\scripts\build.ps1 -Rebuild                 # after project-file changes
.\scripts\run.ps1                            # build and run at repo root
```

The working directory must be the repository root because shaders and textures use relative paths. Output is `x64\<Configuration>\Voyager-2.exe`. Verify changes with the relevant tests in bible section 43, run `scripts\verify_scene_layout.ps1` and `scripts\verify_navigation_and_motion.ps1`, and inspect the rendered result: `Voyager-2.exe --capture <dir>` (or `--capture-bodies <dir>`) writes a screenshot tour and exits. `scripts\fetch_horizons.ps1` regenerates the ephemeris CSVs; the program itself stays offline. When adding a `.cpp` or `.h`, register it in both `Voyager-2.vcxproj` and `Voyager-2.vcxproj.filters`, then rebuild.

## Architecture and Ownership

- `Main.cpp` only constructs, initializes, and runs `Application`. The loop remains `input -> update(dt) -> render`; rendering never mutates simulation state.
- `Window` exclusively owns GLFW/context lifetime. `Input` exclusively polls GLFW and distinguishes held from pressed/released state.
- `Scene` owns root `SceneObject`s; parents own children with `unique_ptr`, while child parent links are non-owning.
- Reusable GPU resources use `shared_ptr`. All bodies share one sphere mesh; do not upload a mesh per body.
- `VAO`, `VBO`, `EBO`, `Mesh`, and `Texture2D` are move-only RAII owners. Do not add raw `new`/`delete` for GPU objects.
- World transforms use `dvec3`/`dquat`; `Renderer::submit` subtracts the camera position in double before narrowing to float (floating origin). Camera movement never edits a body's physical transform.
- `SimulationClock` owns the only date. Planets are positioned from it every frame; moons and spin use a visual clock scaled by the same speed/pause.
- `Application` composes systems; pure geometry algorithms such as `UvSphereGenerator` stay independent of OpenGL.

## Rendering Contract

`Vertex` is eight tightly packed floats: `position` at location 0, `normal` at location 1, and `texCoord` at location 2. `Mesh` and `default.vert` must change together if this format changes. Shader uniforms: `model` (camera-relative), `view` (eye at origin), `proj`, `baseColor`, `useTexture`, `albedoTexture`, `useInstancing`, `shadingModel`, `specularStrength`, `specularPower`, `opacity`, `lightPosition`, `lightColor`, `lightingEnabled`, `logDepthCoefficient`. Every look change goes through `Material` fields, never per-object shader code; see `docs/objects/lighting.md`.

The sphere call is `generate(32, 64)`, producing 2,145 vertices and 3,968 non-degenerate counter-clockwise triangles. The duplicated longitude column is required for the `u=0/1` texture seam.

## Texture Policy

Geometry (vertices, normals, UVs, indices, transforms) must always be self-authored — never import a pre-built mesh or another project's geometry. Textures are a separate pipeline stage (fragment-only; see `docs/objects/phase-2-rendering-pipeline.md`) and may be real, credited photographic imagery: currently the Solar System Scope free 2k pack (CC BY 4.0) and NASA 3D Resources (public domain) — see `assets/textures/bodies/README.md` for the full per-file manifest. `Texture2D::loadFromFile` decodes files via the vendored public-domain `stb_image.h` (same category as GLAD/GLFW — a utility, not geometry/shading logic you'd be graded on). Do not reintroduce a procedural texture generator as the primary path; if one is added later it must be additional, not a replacement for credited real imagery, and must say so in the relevant object docs.

## Solar-System Registry and Scale

`CelestialBodyData` holds physical facts independently of rendering. `SolarSystem` registers parents before children and keeps moons under their planet. `ScaleManager` owns every presentation mapping (power-law radii, capped Sun, heliocentric power law, moon-orbit square root, linear spacecraft metres). A moon's local position and scale are divided by its parent's render radius; `CelestialBody` applies spin only to its own mesh so children are not dragged round. Read `docs/objects/scale-manager.md` and `docs/objects/mission-ephemeris.md` before changing scale, orbits, trajectory data or flyby clearance.

## Object Documentation Contract

`docs/objects/README.md` is the teaching-document index. Every new renderable object must have `docs/objects/<object-id>.md` before its phase is complete. Explain geometry generation, vertex attributes, triangle/index construction, transform order, animation/units, material mapping, asset source and credit, limitations, a diagram or annotated screenshot, and verification steps. Shared mathematics belongs in a shared guide and should be linked from each object file. All 26 current bodies have this doc.

## Coverage Boundary

All bible-named bodies, all four ring systems, and every phase through lighting are present. Known limits (no shadows, moons on a visual clock, compressed distances) are documented in each object page; extend rather than replace.

## Style, Logs, and Commits

Use tabs, Allman braces, one declaration per line, `PascalCase` types, `lowerCamelCase` functions, `m_` members, and `kPascalCase` constants. Use established log prefixes: `[APP]`, `[GLFW]`, `[OPENGL]`, `[SHADER]`, `[ASSET]`, `[SCENE]`, `[SOLAR]`, `[VOYAGER]`, `[TRAJECTORY]`.

Commit summaries are concise, imperative, and focused. Never add Claude, Codex, ChatGPT, or other AI/tool attribution to a commit message. Never add an AI `Co-authored-by`, `Signed-off-by`, or similar trailer. Pull requests should name the bible phase, describe architectural impact, report build/manual tests, and include a screenshot or capture for rendering changes.
