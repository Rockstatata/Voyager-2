# Repository Guidelines

## Project Structure & Source of Truth

`Main.cpp` is the thin entry point. Platform services live in `src/core/`, rendering in `src/rendering/`, and scene ownership/transforms in `src/scene/`. Root `VAO`/`VBO`/`EBO` and shader wrappers are the starter layer. Runtime shaders live in `shaders/` (`scene.*`, `lighting.glsl`, `raytrace*`, `hud.*`) and are combined with `#include` by `ShaderProgram`; catalogs are CSVs in `assets/data/`; local content belongs under `assets/`; dependencies are vendored in `Libraries/`.

Read `docs/Voyager_2_Solar_System_Implementation_Bible.md` before architectural work; it is authoritative. All bible phases are complete: 26 textured bodies, four ring systems, orbit guides, starfield, instanced small-body fields, a comet, and a procedural Voyager 2. Planets and Voyager come from dense offline NASA/JPL Horizons state vectors on one `SimulationClock` date (`MissionEphemeris`); Five shading techniques, three light types, derived lighting maps, ray-traced shadows, a Whitted ray-traced view (F9) with a triangle BVH for Voyager, a NASA-textured Voyager with an Inspect mode, HUD/labels and full camera autonomy are in place. `docs/guide/README.md` is the ground-up learning guide; `docs/objects/README.md` indexes how each piece works.

Register every new source/header in both `Voyager-2.vcxproj` and `Voyager-2.vcxproj.filters`, then rebuild. `ChatGPT.cpp` is uncompiled reference material.

## Build, Test, and Development Commands

Requires Windows, MSVC, the Windows SDK, and **Desktop development with C++**.

```powershell
.\scripts\build.ps1                          # Debug x64
.\scripts\build.ps1 -Configuration Release   # optimized build
.\scripts\build.ps1 -Rebuild                 # after project-file edits
.\scripts\run.ps1                            # build/run from repository root
```

Shader and texture paths require the repository root as working directory. Every change must compile in Debug x64, pass `scripts\verify_scene_layout.ps1` and `scripts\verify_navigation_and_motion.ps1`, and be inspected visually: `Voyager-2.exe --capture <dir>` writes a screenshot tour and exits. Perform relevant manual checks from bible section 43.

## Coding and Architecture Conventions

Use C++20/MSVC style: tabs, Allman braces, one declaration per line, `PascalCase` types, `lowerCamelCase` functions, `m_` members, and `kPascalCase` constants. Preserve `input -> update -> render`; `Window` owns GLFW, `Input` polls it, reusable meshes/textures use RAII and `shared_ptr`, and world transforms remain double precision until `Renderer` subtracts the camera position (floating origin). Prefix diagnostics with categories such as `[APP]`, `[SHADER]`, `[ASSET]`, or `[SCENE]`.

`Vertex` locations are 0 position, 1 normal, and 2 UV. All bodies share the 32×64 UV sphere. Read `docs/objects/` before changing its generation or rendering contract.

## Object Documentation

Every new renderable object requires `docs/objects/<object-id>.md`. Document geometry and triangle construction, transforms and units, material/asset source and credit, limitations, a diagram or annotated screenshot, and verification steps. Update `docs/objects/README.md` and the asset manifest when adding content.

## Commits and Pull Requests

Use concise imperative commits, for example `Add shared UV sphere mesh`. Never add Claude, Codex, ChatGPT, tool attribution, or AI `Co-authored-by`/`Signed-off-by` trailers. PRs should identify the bible phase, explain behavior and architecture, link issues, list build/manual checks, and include captures for visible changes.
