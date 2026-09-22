# Repository Guidelines

## Project Structure & Source of Truth

`Main.cpp` is the thin entry point. Platform services live in `src/core/`, rendering in `src/rendering/`, and scene ownership/transforms in `src/scene/`. Root `VAO`/`VBO`/`EBO` and shader wrappers are the starter layer. Runtime shaders are `default.vert` and `default.frag`; local content belongs under `assets/`; dependencies are vendored in `Libraries/`.

Read `docs/Voyager_2_Solar_System_Implementation_Bible.md` before architectural work; it is authoritative. The pre-lighting object pass now includes 26 textured bodies, educational scale and eccentric motion, four ring systems, orbit guides, starfield, instanced asteroid/Kuiper/Oort fields, a moving comet, and a from-scratch procedural Voyager 2. Historical mode uses 160 offline NASA/JPL Horizons samples with a visible trajectory and mission bookmarks; Manual mode remains available. Lighting/shading and synchronized planetary ephemerides are deliberately deferred. Reuse shared geometry broadly while leaving room for later oblateness and irregular-body improvements.

Register every new source/header in both `Voyager-2.vcxproj` and `Voyager-2.vcxproj.filters`, then rebuild. `ChatGPT.cpp` is uncompiled reference material.

## Build, Test, and Development Commands

Requires Windows, MSVC, the Windows SDK, and **Desktop development with C++**.

```powershell
.\scripts\build.ps1                          # Debug x64
.\scripts\build.ps1 -Configuration Release   # optimized build
.\scripts\build.ps1 -Rebuild                 # after project-file edits
.\scripts\run.ps1                            # build/run from repository root
```

Shader and texture paths require the repository root as working directory. There is no automated suite. Every change must compile in Debug x64 and be exercised with `run.ps1`; perform relevant manual checks from bible section 43 and record results.

## Coding and Architecture Conventions

Use C++20/MSVC style: tabs, Allman braces, one declaration per line, `PascalCase` types, `lowerCamelCase` functions, `m_` members, and `kPascalCase` constants. Preserve `input -> update -> render`; `Window` owns GLFW, `Input` polls it, reusable meshes/textures use RAII and `shared_ptr`, and world transforms remain double precision until GPU upload. Prefix diagnostics with categories such as `[APP]`, `[SHADER]`, `[ASSET]`, or `[SCENE]`.

`Vertex` locations are 0 position, 1 normal, and 2 UV. All bodies share the 32×64 UV sphere. Read `docs/objects/` before changing its generation or rendering contract.

## Object Documentation

Every new renderable object requires `docs/objects/<object-id>.md`. Document geometry and triangle construction, transforms and units, material/asset source and credit, limitations, a diagram or annotated screenshot, and verification steps. Update `docs/objects/README.md` and the asset manifest when adding content.

## Commits and Pull Requests

Use concise imperative commits, for example `Add shared UV sphere mesh`. Never add Claude, Codex, ChatGPT, tool attribution, or AI `Co-authored-by`/`Signed-off-by` trailers. PRs should identify the bible phase, explain behavior and architecture, link issues, list build/manual checks, and include captures for visible changes.
