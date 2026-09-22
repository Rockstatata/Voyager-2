# CLAUDE.md

## Source of Truth

This is a Windows/OpenGL 3.3 Core solar-system explorer built with C++20, MSVC, GLAD, GLFW, and GLM. Read `docs/Voyager_2_Solar_System_Implementation_Bible.md` before architectural work. It is authoritative when guidance conflicts. Section 45 defines the phases, section 43 the manual tests, and sections 49–52 the conventions, failure modes, and canonical interfaces.

`docs/copilot-plan-phase.md` records the completed Phase 1 refactor. `docs/vscode-working.md` is historical; `.vscode/` and `scripts/` are the working configuration. `ChatGPT.cpp` is uncompiled reference material, not production code.

**Current state:** The pre-lighting object pass is complete. `SolarSystem` registers 26 textured bodies (Sun, eight planets, Pluto, and all required/optional moons) sharing one 32×64 indexed UV sphere. `ScaleManager` maps body radius, heliocentric distance, and spacecraft metres independently; bodies spin and follow eccentric educational orbits under pause/speed controls. Four planetary ring systems, eight orbit guides, a 4,000-point starfield, instanced asteroid/Kuiper/Oort fields, and one moving tailed comet are present. Voyager 2 is built entirely from project-native procedural geometry by `VoyagerModelBuilder`: real dish/bus proportions, parabolic HGA/feed/LGA, decagonal bus, three lattice booms, finned RTGs, scan-platform instruments/cameras, magnetometers, PWS antennas, panels, Golden Record, and 16 thrusters. Historical mode interpolates 160 offline NASA/JPL Horizons samples and renders the same path; `1`–`6` select mission bookmarks, `T` toggles the path, `V` switches Historical/Manual, and the default chase camera is a verified three-quarter view. Lighting, shading, synchronized planetary ephemerides, and a full text HUD remain outside this pre-lighting submission.

## Build, Run, and Verify

The project requires MSVC platform toolset `v145`, the Windows SDK, and Visual Studio's **Desktop development with C++** workload. There is no CMake or automated test suite.

```powershell
.\scripts\build.ps1                          # Debug x64
.\scripts\build.ps1 -Configuration Release   # Release x64
.\scripts\build.ps1 -Rebuild                 # after project-file changes
.\scripts\run.ps1                            # build and run at repo root
```

The working directory must be the repository root because shaders and textures use relative paths. Output is `x64\<Configuration>\Voyager-2.exe`. Verify changes with the relevant tests in bible section 43 and record visual results. When adding a `.cpp` or `.h`, register it in both `Voyager-2.vcxproj` and `Voyager-2.vcxproj.filters`, then rebuild.

## Architecture and Ownership

- `Main.cpp` only constructs, initializes, and runs `Application`. The loop remains `input -> update(dt) -> render`; rendering never mutates simulation state.
- `Window` exclusively owns GLFW/context lifetime. `Input` exclusively polls GLFW and distinguishes held from pressed/released state.
- `Scene` owns root `SceneObject`s; parents own children with `unique_ptr`, while child parent links are non-owning.
- Reusable GPU resources use `shared_ptr`. All bodies share one sphere mesh; do not upload a mesh per body.
- `VAO`, `VBO`, `EBO`, `Mesh`, and `Texture2D` are move-only RAII owners. Do not add raw `new`/`delete` for GPU objects.
- World transforms use `dvec3`/`dquat` and narrow to float only at GPU upload. Camera movement never edits a body's physical transform.
- `Application` composes systems; pure geometry algorithms such as `UvSphereGenerator` stay independent of OpenGL.

## Phase 2 Rendering Contract

`Vertex` is eight tightly packed floats: `position` at location 0, `normal` at location 1, and `texCoord` at location 2. `Mesh` and `default.vert` must change together if this format changes. Shader uniforms are `model`, `view`, `proj`, `baseColor`, `useTexture`, and `albedoTexture`.

The sphere call is `generate(32, 64)`, producing 2,145 vertices and 3,968 non-degenerate counter-clockwise triangles. The duplicated longitude column is required for the `u=0/1` texture seam.

## Texture Policy

Geometry (vertices, normals, UVs, indices, transforms) must always be self-authored — never import a pre-built mesh or another project's geometry. Textures are a separate pipeline stage (fragment-only; see `docs/objects/phase-2-rendering-pipeline.md`) and may be real, credited photographic imagery: currently the Solar System Scope free 2k pack (CC BY 4.0) and NASA 3D Resources (public domain) — see `assets/textures/bodies/README.md` for the full per-file manifest. `Texture2D::loadFromFile` decodes files via the vendored public-domain `stb_image.h` (same category as GLAD/GLFW — a utility, not geometry/shading logic you'd be graded on). Do not reintroduce a procedural texture generator as the primary path; if one is added later it must be additional, not a replacement for credited real imagery, and must say so in the relevant object docs.

## Solar-System Registry and Scale

`CelestialBodyData` holds physical facts independently of rendering. `SolarSystem` registers parents before children and keeps moons under their planet. `ScaleManager` now owns all educational mappings; `CelestialBody` applies axial spin and eccentric orbit motion. Read `docs/objects/scale-manager.md` and `docs/objects/orbital-motion.md` before changing parent-scale compensation or orbital units.

## Object Documentation Contract

`docs/objects/README.md` is the teaching-document index. Every new renderable object must have `docs/objects/<object-id>.md` before its phase is complete. Explain geometry generation, vertex attributes, triangle/index construction, transform order, animation/units, material mapping, asset source and credit, limitations, a diagram or annotated screenshot, and verification steps. Shared mathematics belongs in a shared guide and should be linked from each object file. All 26 current bodies have this doc.

## Coverage Boundary

All bible-named bodies and all four ring systems are present. Spherical bodies intentionally share one mesh; later geometry work may add oblateness/irregular silhouettes without changing the registry. Remaining astronomy accuracy work is synchronized date-based planetary ephemerides and higher-frequency Voyager sampling, not missing object categories.

## Style, Logs, and Commits

Use tabs, Allman braces, one declaration per line, `PascalCase` types, `lowerCamelCase` functions, `m_` members, and `kPascalCase` constants. Use established log prefixes: `[APP]`, `[GLFW]`, `[OPENGL]`, `[SHADER]`, `[ASSET]`, `[SCENE]`, `[SOLAR]`, `[VOYAGER]`, `[TRAJECTORY]`.

Commit summaries are concise, imperative, and focused. Never add Claude, Codex, ChatGPT, or other AI/tool attribution to a commit message. Never add an AI `Co-authored-by`, `Signed-off-by`, or similar trailer. Pull requests should name the bible phase, describe architectural impact, report build/manual tests, and include a screenshot or capture for rendering changes.
