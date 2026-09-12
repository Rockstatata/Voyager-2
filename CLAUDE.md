# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A Windows/OpenGL 3.3 Core solar-system explorer ("Voyager 2 Explorer"), built from an instructor starter project (VAO/VBO/EBO/Shader wrappers, GLAD, GLFW, glm) and now growing into the architecture specified in `docs/Voyager_2_Solar_System_Implementation_Bible.md` — the master spec for this project. Read it before any architectural change; it is authoritative over anything below where they'd conflict. Section 45 tracks phase-by-phase scope, section 52 has canonical class interfaces, section 49 has naming/ownership/logging conventions, section 51 lists failure modes to avoid (e.g. F1: everything in main.cpp, F8: one VAO/VBO created every frame, F9: one draw call per asteroid).

`docs/copilot-plan-phase.md` is the plan that produced the current `src/core` and `src/rendering/Camera` layer (Phase 1) — historical record, already executed, not a pending task list. `docs/vscode-working.md` is superseded by the `.vscode/` config and `scripts/` described below; the MSBuild/tasks.json approach it recommends is what's actually wired up now.

**Current state:** Phase 0 and Phase 1 complete — `Application` owns `Window`/`Input`/`Time`/`Camera`/`Renderer`/`Scene` and runs the loop; two `SceneObject`s share one uploaded `Mesh` via `shared_ptr`; free-fly `Camera` reproduces the starter's exact original view. Next up is Phase 2 (UV sphere + `Vertex{position, normal, uv}` + depth-tested multi-body render — bible section "Phase 2").

## Build, run, debug

Windows-only, MSVC toolchain (`v145` platform toolset, C++20). Requires the **Desktop development with C++** workload (MSVC compiler, Windows SDK, MSBuild) — no CMake, no ninja.

From VS Code, opened at this folder:
- **Ctrl+Shift+B** — Debug x64 build (default task).
- **F5** — build then launch under the Visual Studio debugger (`cppvsdbg`), breakpoints work.
- Other tasks via Command Palette → *Run Task*: `Build Release x64`, `Rebuild Debug x64`, `Run Debug x64`.

From a plain terminal (no Developer Prompt needed — the scripts locate MSBuild via `vswhere` themselves):
```powershell
.\scripts\build.ps1                          # Debug x64
.\scripts\build.ps1 -Configuration Release
.\scripts\build.ps1 -Rebuild                 # full rebuild; needed after editing the .vcxproj
.\scripts\run.ps1                            # build, then run from the project root
```
Output lands at `x64\<Configuration>\Voyager-2.exe`.

**Working directory matters.** `Shader` (`shaderClass.cpp`) loads `default.vert`/`default.frag` by relative path and throws a bare `errno` if the cwd is wrong. The exe must run with the project root as cwd — `scripts/run.ps1` and `.vscode/launch.json` both set this; don't launch the `.exe` directly from `x64\Debug\`.

**No automated test suite.** Bible section 43 defines the test plan (T1–T11) as manual/visual checks — window opens, shapes render, transforms are correct, FPS is stable, etc. Verify a change by building, running, and checking against the relevant T-numbered criterion.

**Adding a new source file** requires registering it in *both* `Voyager-2.vcxproj` (`ClCompile`/`ClInclude` ItemGroups) and `Voyager-2.vcxproj.filters` (for VS Solution Explorer grouping) — MSBuild does not glob. `Rebuild Debug x64` after any such change; an incremental build can miss a project-file edit.

## Architecture

**Loop ownership (bible section 10, Rule 4).** `Application` (`src/core/Application.*`) is the only thing `Main.cpp` touches: `initialize()` then `run()`. Its loop is strictly `input → update(dt) → render()` — `render()` must never mutate scene/simulation state, and objects must never touch OpenGL from inside `update()`. `Window` is the sole owner of the GLFW window/context lifetime and the only class that calls GLFW window/context APIs directly; `Input` is the sole reader of `glfwGetKey`/`glfwGetMouseButton`, exposing `keyDown` (continuous — thrust, steering) vs `keyPressed`/`keyReleased` (edge-triggered — mode switches, toggles) so no other code queries GLFW state directly.

**Scene graph.** `Scene` owns root `SceneObject`s; each owns children via `unique_ptr` and computes `worldMatrix()` by walking to its (non-owning) parent pointer. A `Mesh` is held by `shared_ptr` on `SceneObject`, not by value — geometry is meant to be shared (one sphere mesh for every planet), and duplicating uploads per-body is exactly the Phase-2+ trap the bible warns against (F9).

**Double-precision transforms (bible section 11–12, Rule 6).** `Transform` (`src/scene/Transform.h`) keeps `position`/`rotation`/`scale` in `dvec3`/`dquat` and only narrows to `float` in `modelMatrix()` at upload time. `Camera` does the same — position is `dvec3`, narrowed to `vec3` only inside `viewMatrix()`. This is the seam the later floating-origin phase (bible section 20) will hook into; don't introduce `float` world positions elsewhere. Moving the camera must never write to a body's physical `Transform` — the camera and the simulation are separate systems by construction (`Renderer::submit` never touches `Transform`, `Camera::update` never touches `Scene`).

**Renderer holds no camera.** `Renderer::beginFrame(camera, aspectRatio)` takes the camera each frame instead of caching a view/proj internally — this was the Phase 1 refactor. It caches GL uniform locations per shader (`model`/`view`/`proj`/`scale`) rather than re-resolving them every `submit()`.

**GPU resource ownership.** `VAO`/`VBO`/`EBO` (project root) are move-only RAII: constructor allocates, destructor frees, `Delete()` is idempotent and safe to call early. `Mesh` (`src/rendering/Mesh.*`) holds them *by value*, not by pointer — no `new`/`delete` inside `Mesh`. When binding EBOs, always bind the VAO first (element-array binding is captured as VAO state) and unbind the VAO before unbinding the EBO, or the binding is lost — see the comment in `Mesh::Mesh`.

**Shader contract.** `default.vert`/`default.frag` expect uniforms `model`, `view`, `proj`, `scale` and vertex attributes `layout(location=0) vec3 aPos`, `layout(location=1) vec3 aColor`. Any new mesh format (normals, UVs — Phase 2) needs a shader update in lockstep; `Mesh`'s attribute layout and the `.vert`'s `layout(location=...)` must agree.

**Logging categories** (bible section 49): `[APP] [GLFW] [OPENGL] [SHADER] [ASSET] [SCENE] [SOLAR] [VOYAGER] [TRAJECTORY]` — `std::cout` prefixed, no logging framework. Follow this when adding diagnostics.

`ChatGPT.cpp` is a scratch/reference file (procedural sphere generation) — not part of the build (`Voyager-2.vcxproj` doesn't compile it); consult it for sphere-generation approach when starting Phase 2, don't build on it directly.

`assets/{data,models,shaders,textures,trajectory}/` are empty scaffolding for later phases (solar-system JSON, trajectory CSV, textures — bible section 44) — nothing loads from them yet.
