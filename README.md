# Voyager-2

A from-scratch 3D solar system explorer built on modern OpenGL (3.3 Core), C++20, and MSVC — free-fly camera, real-time depth-tested rendering, and a scene graph designed to grow toward a full photorealistic solar system: every planet and major moon, ring systems, orbital planes, the asteroid belt, Kuiper belt, Oort cloud, and a dense background starfield.

Started from an instructor starter project (VAO/VBO/EBO/Shader wrappers, GLAD, GLFW, glm) and is being rebuilt into the architecture specified in [docs/Voyager_2_Solar_System_Implementation_Bible.md](docs/Voyager_2_Solar_System_Implementation_Bible.md) — the authoritative spec for this project.

## Status

**Phase 0–1 complete:** `Application` owns `Window` / `Input` / `Time` / `Camera` / `Renderer` / `Scene` and drives the loop; scene objects share uploaded meshes via `shared_ptr`; a free-fly `Camera` supports double-precision world positions (`dvec3`/`dquat`), narrowed to `float` only at GPU upload time.

**Phase 2 (in progress):** procedural UV sphere geometry, full vertex format (`position` / `normal` / `uv`), multi-body depth-tested rendering.

See the implementation bible (linked above) for the full phase breakdown and architectural rules this project follows.

## Build & run

Windows only, MSVC toolchain (`v145` platform toolset, C++20). Requires the **Desktop development with C++** workload (MSVC compiler, Windows SDK, MSBuild) — no CMake, no ninja.

### VS Code (no Visual Studio needed)

Open this folder in VS Code, then:

- **Ctrl+Shift+B** — Debug x64 build (default task)
- **F5** — build, then launch under the debugger (breakpoints work)
- Command Palette → *Run Task* for `Build Release x64`, `Rebuild Debug x64`, `Run Debug x64`

### Terminal

```powershell
.\scripts\build.ps1                          # Debug x64
.\scripts\build.ps1 -Configuration Release
.\scripts\build.ps1 -Rebuild                 # full rebuild; needed after editing the .vcxproj
.\scripts\run.ps1                            # build, then run from the project root
```

These scripts locate MSBuild via `vswhere` themselves — no Developer Command Prompt required.

Output lands at `x64\<Configuration>\Voyager-2.exe`. **Run it via `scripts/run.ps1` or the VS Code launch config**, not directly from `x64\Debug\` — the executable loads shaders by a path relative to the project root, so the working directory matters.

## Controls

- `W` / `A` / `S` / `D` — move
- `Space` / `Left Ctrl` — up / down
- `Left Shift` / `Right Shift` — sprint
- `Esc` — quit

## Architecture

- `Window` is the sole owner of the GLFW window/context; `Input` is the sole reader of raw GLFW input state, exposing `keyDown` (continuous) and `keyPressed` / `keyReleased` (edge-triggered).
- `Scene` owns a tree of `SceneObject`s; each computes its world matrix from a non-owning parent pointer. Geometry (`Mesh`) is held by `shared_ptr` and reused across bodies — one sphere upload serves every planet and moon.
- `Transform` and `Camera` keep world position in double precision (`dvec3` / `dquat`), narrowing to `float` only when uploading to the GPU — the seam a later floating-origin phase will use.
- `Renderer` takes the camera per frame (`beginFrame(camera, aspectRatio)`) rather than caching one internally, and caches shader uniform locations rather than re-resolving them every draw call.

Full conventions (logging categories, naming, failure modes to avoid) are in the bible, sections 49 and 51.

## No automated tests

This project is verified manually against the bible's test plan (section 43) — build, run, and check the change against the relevant criterion.
