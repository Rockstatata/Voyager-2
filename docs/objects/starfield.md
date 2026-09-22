# Background Starfield

The exact point-position equation and index construction are also collected in the [Procedural Mesh Construction Handbook](procedural-meshes.md).

## Overview

`starfield`, a single `SceneObject` at the scene root, drawn as `GL_POINTS`. Bible section 30 ("Background Stars"): "a skybox or star sphere is enough" for current scope, and "camera translation should not make the skybox visibly move" — satisfied here simply by making the shell radius (80) much larger than any distance the free-fly camera or cruising Voyager currently travels, not by any special re-centering logic.

## Geometry generation

`StarfieldGenerator::generate(count, radius)` (`src/rendering/StarfieldGenerator.*`) places `count` points uniformly on a sphere shell of the given radius. Uniform-on-a-sphere needs care: sampling the polar angle (`phi`) directly bunches points near the poles, the same non-uniform-density trap `UvSphereGenerator`'s *latitude rings* fall into (and correct for with geometry, since a sphere mesh needs an even grid). A point cloud has no grid to correct with, so the fix is in the sampling instead: sample `cos(phi)` uniformly in `[-1, 1]` rather than `phi` itself, which is the standard "uniform point on a sphere" formula. Fixed seed (`std::mt19937`), so the sky is deterministic between runs.

4,000 stars, radius 80 render units — comfortably inside the camera's 200-unit far plane, and larger than the current planet line's extent (roughly ±14 in Z, ±7 in X) so the whole solar system sits inside the shell.

## Vertex attributes / triangle construction

Standard `Vertex{position, normal, uv}` struct (so it fits the one vertex format this whole project uses), but `normal` and `uv` are unused for a point — there's no surface to shade or texture-map, just a position. No triangles: the index buffer is `0, 1, 2, ..., count-1` and `Mesh::draw()` is called with `PrimitiveMode::Points`, so `glDrawElements(GL_POINTS, ...)` treats each index as one independent point rather than assembling triangles from them. This needed a small `Mesh`/shader-adjacent change — see `Mesh.h`'s `PrimitiveMode` enum — to let a `Mesh` be drawn as something other than triangles at all, alongside the (separate) instancing change used by the belts.

## Transform

Scene root, identity transform (position at the world origin, unscaled) — the shell radius is baked into the generated vertex positions themselves, not applied via a scale.

## Material mapping

Flat off-white color, no texture. Point size is fixed at 2 pixels via `glPointSize(2.0f)` (a GL render-state call in `Renderer::beginFrame`, not a shader feature) — deliberately not per-vertex `gl_PointSize`, which needs `GL_PROGRAM_POINT_SIZE` enabled and extra shader work for no benefit at this stage, since every star is the same size.

## Limitations

- All stars are a uniform size, brightness, and color — no magnitude variation, no color temperature.
- No twinkle/blink animation (would need either per-vertex brightness varying in the fragment shader, which is shading work out of scope for this pass, or a coarse whole-field brightness pulse, which wasn't judged worth the visual payoff here).
- Centered on the world origin, not on the camera — fine at the current scene scale (80-unit radius vs a scene that fits inside ~±15 units), but would need re-centering if the camera ever travels far enough to approach the shell radius.

## Verification

- `[SCENE] environment built: starfield (4000 stars), ...` startup log line.
- Visual: background reads as a field of small points surrounding the whole scene from any camera angle, not visibly shifting as the camera moves through the planet line.
