# Controls: Camera Modes, Focus, and the Simulation Clock

## Camera modes

`H` or `Home` moves FreeFly to the scientific overview camera: 110 world units from the Sun, looking toward the complete compressed heliocentric system. Bodies keep exact mutual size ratios; use `Tab` to inspect one at close range.

The application starts in `ThirdPerson` so Voyager is immediately visible from directly behind. `C` is a direct fixed/free toggle: any locked mode (`ThirdPerson` or body `Focus`) switches to `FreeFly`, and `FreeFly` switches back to `ThirdPerson`. Body `Focus` is entered separately with `Tab`, so camera behavior is predictable during a demonstration.

- **FreeFly**: the original Phase-1 camera — RMB + WASD, Space/Ctrl, Shift boost. Untouched by everything since.
- **ThirdPerson**: follows Voyager 2 from behind by default. Hold **RMB** and move the mouse to orbit freely around the spacecraft; the probe remains centred in view in both Historical and Manual flight modes. Releasing RMB restores the cursor, while holding it captures the cursor so mouse-look cannot stop at a screen edge.
- **Focus**: locked on whatever body is currently selected (see below). Reuses `Camera::followTarget` — the same method ThirdPerson uses — with a fixed elevated-diagonal offset direction instead of a moving ship's heading, since a planet has no "facing" direction of its own to frame from.

## Object selection ("Focus")

`Tab` / `Shift+Tab` cycle forward/backward through every body in `SolarSystem` (Sun, planets, moons — in registration order) and switch the camera into Focus mode on the newly-selected one. This is bible section 40's "object selection and focus," in minimal form: keyboard cycling plus a camera snap. The selected name appears in the native window-title HUD and in the `[APP] focused: <name>` console log. In-scene floating labels still require a font atlas and text shader, so they remain part of the later shader/presentation pass.

Focus distance is derived per-body, not a fixed number: `Camera::followTarget` is given `worldRadius * 4 + 0.05` as its distance, where `worldRadius` is the length of the focused body's transformed X basis vector — `glm::length(glm::dvec3(focused->worldMatrix()[0]))`. This matters specifically for moons: a moon's own `transform().scale` is *local* (relative to its parent planet — see [scale-manager.md](scale-manager.md)'s cascade explanation), not its actual on-screen size, so reading `transform().scale` directly would compute a wildly wrong distance for any moon. Extracting the scale from the body's full `worldMatrix()` instead is correct for both a root (a planet) and a child (a moon) uniformly.

## Simulation clock (bible section 21)

`P` toggles pause. `=` (or `]`) speeds up, `-` (or `[`) slows down, each by 1.5x per press, clamped to [0.02x, 50x]. Historical playback stops at the final mission date instead of teleporting back to launch.

Implementation: `CelestialBody::setSimulationTimeScale` is a **shared static** — one call reaches every `CelestialBody` instance at once, since every planet and moon is its own object with no back-reference to a central clock. `CelestialBody::update` multiplies incoming `dt` by that shared scale before advancing orbit angle or spin angle: `simDt = dt * s_simulationTimeScale`. The same scale drives Voyager's historical trajectory playback; manual piloting and the camera stay on real time. Pausing sets the scale to 0 (motion literally stops advancing, not just rendered statically) and restores the last non-zero speed on unpause.

**Deliberately not affected**: `Input`, camera movement, and Voyager's manual piloting use real, unscaled `deltaTime`. Historical Voyager playback does use the simulation scale, so pause and speed changes apply consistently to the automated mission path.

## Verification

- `[APP] camera mode: ...` log line on every `C` press.
- `[APP] focused: <displayName>` log line on every `Tab`/`Shift+Tab` press; visual: camera immediately snaps to look at that body, sized appropriately whether it's a planet or a small moon.
- `[APP] simulation paused` / `resumed` / `simulation speed: Nx` log lines; visual: all orbital and spin motion stops on pause and resumes at the logged rate — camera movement and Voyager piloting remain responsive throughout.

## Voyager trajectory controls

- `T` shows/hides the historical path line.
- `1` through `6` jump to Launch, Jupiter, Saturn, Uranus, Neptune, and Interstellar-space dates. A bookmark switches to Historical + ThirdPerson mode and logs its name.

## Minimal HUD

The native window title is a shader-free HUD. It refreshes four times per second and shows camera mode, Historical/Manual flight mode, current historical Julian Date, Voyager's scene-space distance from the Sun and velocity, simulation running/paused state and speed, plus the focused body's name when Focus is active. This deliberately avoids adding a font atlas and screen-space text shader before the lighting/shading lab.
