# 10. How to change things

[Guide index](README.md) · previous: [Data, time and the mission](09-mission-data.md)

Step-by-step recipes for the changes you are most likely to be asked to make. After any change: build (`.\scripts\build.ps1`), run it and look, then run both verify scripts.

## Colours, shininess and look

### Make a planet shinier or duller
All bodies with the same preset share one setting. Edit the preset's values in `MaterialLibrary::surface` (src/rendering/MaterialLibrary.cpp), for example `ice`: `specularStrength = 0.22f; specularPower = 36.0f;`. Strength is highlight brightness (0..1); power is tightness (higher = smaller highlight). To change one body only, give it its own preset name in the `material` column of `celestial_bodies.csv` and add a branch for that name.

### Change a Voyager part's colour or finish
In `VoyagerModelBuilder::build`, each finish is one line:

```cpp
const auto goldFoil = finish({ 390, 710, 490, 818 }, glm::vec3(1.15f), 0.75f, 70.0f);
//                           atlas pixels l,t,r,b      tint (×)         spec   power
```

- Change the **tint** to brighten, darken or recolour (it multiplies the photo).
- Change the **rectangle** to show a different part of `voyager_nasa_atlas.png`. Open the image in any editor and read the pixel corners.
- To swap which finish a part uses, change the material argument of its `addBox` or `addCylinder` call.

### Ambient brightness, headlamp cone, fill direction
`LightingController::build` (src/core/LightingController.cpp): `state.ambient = 0.07f`; the headlamp's `innerCutoffCos` and `outerCutoffCos` (12° and 18°); the fill's `direction` and `color`; the Sun's `attenuation`.

### Softer or harder shadows
`RayTraceScene::sunLightRadius` (src/rendering/RayTraceScene.h, default 0.6). A larger radius means a larger light and softer penumbrae; 0 means hard shadows.

## Shading

### Change toon bands or outline
`toonBand` in shaders/lighting.glsl (thresholds 0.75, 0.40, 0.12 and levels 1.0, 0.62, 0.30, 0.06). The outline is `if (rim < 0.22) lit *= 0.15;` in shaders/scene.frag. Shaders are read at start-up, so restart the program; there is no need to rebuild C++.

### Add a sixth shading technique
1. `src/rendering/Lighting.h`: add it to `enum class ShadingTechnique` (for example `Minnaert = 5`) and a name in `shadingTechniqueName`.
2. `shaders/lighting.glsl`: `#define SHADING_MINNAERT 5`, and in `evaluateLights` compute its diffuse and specular under `if (technique == SHADING_MINNAERT)`.
3. `src/core/LightingController.cpp`: in `handleKeys`, change `const int count = 5;` to 6.
4. If it needs a different normal or per-vertex work, add a branch in scene.frag and scene.vert as Flat and Gouraud do.
5. Add a shot for it in the `--capture-shading` list in `Application.cpp`, capture it, and add the image to chapter 6.

### Add another light
`kMaxLights = 4` and lights 0–2 are used, so slot 3 is free. In `LightingController::build`, fill `state.lights[3]` (type, position or direction, colour, intensity, attenuation, cone). Give it a key in `handleKeys`. The shaders loop over every enabled light automatically. Only light 0 (the Sun) casts shadows.

## Scene content

### Add a moon or a body
1. Add a row to `assets/data/celestial_bodies.csv`: `id,name,parent,radius_km,semi_major_axis_km,eccentricity,orbital_period_days,rotation_period_hours,axial_tilt_deg,texture,material,type`. A moon gives its planet's id as `parent`, and parents must come before children.
2. Put the texture (equirectangular JPG) in `assets/textures/bodies/` and add its source and licence to that folder's README.
3. Write `docs/objects/<id>.md` (required by the project's documentation contract).
4. A new *planet* also needs `assets/trajectory/planets/<id>_heliocentric.csv`. Run `scripts/fetch_horizons.ps1` to fetch it.

The ray tracer handles up to 32 spheres (`MAX_SPHERES` in raytrace.glsl and `RayTraceScene::kMaxSpheres`). There are 26 today.

### Add or change a ring band
Add a row to `assets/data/ring_bands.csv`: `planet,band,inner,outer,r,g,b,opacity`, with radii in units of the planet's radius. Both raster and ray-traced views pick it up (limit: 16 bands).

### Add a Voyager part
In `VoyagerModelBuilder::build`, choose a helper and give dimensions in metres through `units()`:

```cpp
addBox("voyager2_my_box", position, { units(0.2), units(0.1), units(0.1) }, whitePaint);
addCylinder("voyager2_my_tube", centre, axisDirection, units(0.05), units(0.05), units(0.4), 16, aluminium);
component("My part", "One-line fact shown in Inspect.", centre, 0.4);   // optional: make it inspectable
```

Going through `add` also puts the part in the ray tracer's BVH automatically. Check it with `I`, `.` / `,`, F3 → Flat (the triangles) and F9 (the ray tracer shows any overlap).

### Add a new geometry generator
1. Create `src/rendering/MyGenerator.h/.cpp` with `static MeshData generate(...)`, using no OpenGL calls.
2. Follow chapter 2: vertices, unit outward normals, UVs, counter-clockwise indices (and duplicate the seam if it wraps).
3. **Register both files in `Voyager-2.vcxproj` and `Voyager-2.vcxproj.filters`**, then `.\scripts\build.ps1 -Rebuild`.
4. Document it in `docs/objects/procedural-meshes.md`.

## Controls and camera

### Add or change a key
- Lighting keys: `LightingController::handleKeys`.
- Everything else: `Application::handleKeys` (use `m_input.keyPressed` for one-shot actions, `keyDown` for held keys).
- Free-flight movement keys: `Camera::updateFreeFly`. Manual Voyager keys: `Voyager2::applyManualControl`.
- Then update the F1 help text (`HudOverlay`), `docs/objects/controls.md` and the root README.

### Camera speed and zoom limits
- Free-flight speed: `std::clamp(nearestSurfaceDistance(...) * 0.9, 0.004, 300.0)` in `CameraController::update`.
- Closest focus distance: `radius * 1.08` (Focus) and `size * 0.3` (Inspect) in the same function.
- Zoom step per wheel notch: `0.85` in `Camera::updateOrbit`.

## Time and scale

- Default playback rate: `kBaseDaysPerSecond = 120.0` (SimulationClock.h).
- Encounter slow-motion window and strength: `kEncounterWindowDays`, `kMinimumEncounterFactor`.
- Planet sizes and distances: the constants in `ScaleManager.h`. Read [scale-manager.md](../objects/scale-manager.md) first, and run `scripts/verify_scene_layout.ps1` afterwards; it checks that moons clear rings and flybys clear planets.
- Voyager's size: `kSpacecraftUnitsPerMetre` (0.006). Update the check in `scripts/verify_scene_layout.ps1` to match.

## When something goes wrong

| Symptom | Likely cause |
| --- | --- |
| Console says `[SHADER] failed to read ...` | The working directory is not the repository root. Use `scripts/run.ps1`. |
| `[SHADER] ... compile error` | A GLSL syntax error. The log gives the file and line after `#include` expansion. |
| An object is invisible or inside-out | The triangle winding is clockwise (it is culled). Swap two indices per triangle. |
| Lighting looks inverted on a part | The normals point inward. Negate them, or check the normal matrix for mirrored scales. |
| Speckled dark dots on Voyager | Shadow acne: raise the `2e-5` bias in scene.frag and raytrace.frag. |
| A texture looks mirrored or upside down | UV direction; see chapter 4.1 (flip) and 2.2 (`1 − u`). |
| Flickering far surfaces | Something bypasses the log-depth write (a new shader must write `gl_FragDepth` the same way). |
| Linker error for a new file | It is not registered in `Voyager-2.vcxproj`. |
