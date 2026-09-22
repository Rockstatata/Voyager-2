# Voyager-2

A from-scratch 3D solar system explorer built on modern OpenGL (3.3 Core), C++20, and MSVC — free-fly camera, real-time depth-tested rendering, and a scene graph designed to grow toward a full photorealistic solar system: every planet and major moon, ring systems, orbital planes, the asteroid belt, Kuiper belt, Oort cloud, and a dense background starfield.

Started from an instructor starter project (VAO/VBO/EBO/Shader wrappers, GLAD, GLFW, glm) and is being rebuilt into the architecture specified in [docs/Voyager_2_Solar_System_Implementation_Bible.md](docs/Voyager_2_Solar_System_Implementation_Bible.md) — the authoritative spec for this project.

## Status

**Pre-lighting object pass complete:** `SolarSystem` registers 26 textured bodies: the Sun, eight planets, Pluto, all bible-required moons, and four optional Saturnian moons. They share one indexed 32×64 UV-sphere mesh, use real physical metadata, educational radius/distance mapping, axial spin, and eccentric orbital paths.

The environment also contains four procedural ring systems, planet orbit guides, termination-shock/heliopause wireframes, a 4,000-point starfield, instanced asteroid/Kuiper/Oort fields, and a moving tailed comet. Voyager 2 is authored from project-native geometry—parabolic dish, decagonal bus, lattice booms, finned RTGs, scan-platform instruments, antennas, panels, Golden Record, and 16 thrusters—with NASA assets used only as reference. Historical mode follows a checked-in NASA/JPL Horizons trajectory and draws its path offline. Lighting and shading are intentionally deferred.

See the implementation bible for the full roadmap and [`docs/objects/`](docs/objects/) for illustrated, object-by-object implementation explanations.

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

- `C` — toggle the fixed ThirdPerson chase camera / FreeFly camera
- `Tab` / `Shift+Tab` — focus next / previous celestial body
- `V` — toggle Voyager Historical / Manual flight
- `T` — show/hide the Voyager trajectory
- `1`–`6` — jump to Launch, Jupiter, Saturn, Uranus, Neptune, or Interstellar bookmarks
- `W` / `S`, `A` / `D` — thrust and yaw in Manual mode; move in FreeFly
- `Space` / `Left Ctrl` — vertical thrust in Manual mode; up/down in FreeFly
- `P` — pause/resume celestial motion
- `=` / `-` (or `]` / `[`) — increase/decrease simulation speed
- `Right mouse` — look around in FreeFly; `Shift` boosts camera speed
- `Esc` — quit

The window title acts as a minimal no-shader HUD for camera/flight mode, historical Julian Date, Voyager distance/speed, simulation speed/pause, and focused body.

## Architecture

- `Window` is the sole owner of the GLFW window/context; `Input` is the sole reader of raw GLFW input state, exposing `keyDown` (continuous) and `keyPressed` / `keyReleased` (edge-triggered).
- `Scene` owns a tree of `SceneObject`s; each computes its world matrix from a non-owning parent pointer. Geometry (`Mesh`) is held by `shared_ptr` and reused across bodies — one sphere upload serves every planet and moon.
- `Transform` and `Camera` keep world position in double precision (`dvec3` / `dquat`), narrowing to `float` only when uploading to the GPU — the seam a later floating-origin phase will use.
- `Renderer` takes the camera per frame (`beginFrame(camera, aspectRatio)`) rather than caching one internally, and caches shader uniform locations rather than re-resolving them every draw call.

Full conventions (logging categories, naming, failure modes to avoid) are in the bible, sections 49 and 51.

## No automated tests

This project is verified manually against the bible's test plan (section 43) — build, run, and check the change against the relevant criterion.
