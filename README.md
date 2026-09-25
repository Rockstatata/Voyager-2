# Voyager-2

A from-scratch 3D solar system explorer built on modern OpenGL (3.3 Core), C++20, and MSVC — free-fly camera, real-time depth-tested rendering, and a scene graph designed to grow toward a full photorealistic solar system: every planet and major moon, ring systems, orbital planes, the asteroid belt, Kuiper belt, Oort cloud, and a dense background starfield.

Started from an instructor starter project (VAO/VBO/EBO/Shader wrappers, GLAD, GLFW, glm) and is being rebuilt into the architecture specified in [docs/Voyager_2_Solar_System_Implementation_Bible.md](docs/Voyager_2_Solar_System_Implementation_Bible.md) — the authoritative spec for this project.

## Status

**All bible phases complete.** 26 textured bodies (Sun, eight planets, Pluto and every required and optional moon) share one indexed 32×64 UV sphere. Every planet and Voyager 2 is placed from dense NASA/JPL Horizons state vectors on one shared simulation date, so all four giant-planet flybys happen at the real time (within a minute) and at the real planet-relative geometry (within 0.5 %). Voyager 2 is built entirely from project-native geometry. Sun lighting, glow, translucent rings, a floating origin, logarithmic depth, a HUD with labels, and full free-flight camera control are in place.

![Voyager 2 six hours before Jupiter closest approach](docs/objects/images/runtime/03_jupiter_approach.jpg)

See the implementation bible for the roadmap and [`docs/objects/`](docs/objects/) for illustrated, object-by-object explanations with runtime captures.

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

Press **F1** in the program for the full list ([docs/objects/controls.md](docs/objects/controls.md)).

- **Free flight** (`C`, or any fly key from a locked view): `W A S D`, `Space`/`E` up, `Ctrl`/`Q` down, RMB-drag or `M` mouse-look, arrows turn. **Wheel** sets cruise speed; speed also adapts to the distance from the nearest surface. `Shift` fast, `Alt` fine.
- **Focus**: `Tab` / `Shift+Tab` fly to each of the 26 bodies; drag to orbit, wheel to zoom from the surface out to the whole system; `G` returns to the selection; `H`/`Home` shows the overview.
- **Voyager**: `V` Historical/Manual. Manual (chase camera): `W`/`S` thrust, `A`/`D` yaw, `R`/`F` pitch, `Q`/`E` roll, `Space`/`Ctrl` up/down, `Shift` boost, `X` brake.
- **Mission**: `1` launch, `2`–`5` Jupiter/Saturn/Uranus/Neptune flybys (with automatic encounter slow-motion), `6` heliopause. `P` pause, `=`/`-` speed, `Backspace` reset speed, `N` slow-motion toggle, `T` trajectory, `O` orbit guides.
- **Display**: `L` labels, `K` lighting, `F2` HUD, `F12` screenshot, `Esc` quit.

`Voyager-2.exe --capture <dir>` and `--capture-bodies <dir>` write scripted screenshot tours (used for the documentation).

## Architecture

- `Window` is the sole owner of the GLFW window/context; `Input` is the sole reader of raw GLFW input state, exposing `keyDown` (continuous) and `keyPressed` / `keyReleased` (edge-triggered).
- `Scene` owns a tree of `SceneObject`s; each computes its world matrix from a non-owning parent pointer. Geometry (`Mesh`) is held by `shared_ptr` and reused across bodies — one sphere upload serves every planet and moon.
- `Transform` and `Camera` keep world position in double precision (`dvec3` / `dquat`). `Renderer` subtracts the camera position in double before narrowing to `float` (floating origin), and the shaders write logarithmic depth.
- `MissionEphemeris` and `SimulationClock` own the single mission date; planets and Voyager are both evaluated from Horizons state vectors at that date.
- `Renderer` takes the camera per frame (`beginFrame(camera, aspectRatio)`) rather than caching one internally, and caches shader uniform locations rather than re-resolving them every draw call.

Full conventions (logging categories, naming, failure modes to avoid) are in the bible, sections 49 and 51.

## Verification

There is no unit-test framework. Changes are verified against the bible's test plan (section 43), with the scripted capture tours, and with two offline regression scripts:

```powershell
.\scripts\verify_scene_layout.ps1          # scale, clearances, Horizons tables, flyby geometry vs NASA values
.\scripts\verify_navigation_and_motion.ps1 # clock, camera autonomy, 6-DOF flight, rendering contracts
```

`scripts/fetch_horizons.ps1` regenerates the offline ephemeris tables (network needed only for that script).
