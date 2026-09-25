# GPU Instancing (shared mechanism)

Used by the asteroid belt, Kuiper belt, and Oort cloud (`small-body-fields.md`) — the shared math and engine mechanism lives here so it's explained once, not three times, per the Object Documentation Contract's rule for shared algorithms.

## The problem this solves

Bible failure mode F9: "thousands of asteroid `SceneObject` draw calls." A naive belt — one `SceneObject`, one `Mesh`, one `glDrawElements` call per rock — would submit 2,500+ draw calls a frame for the asteroid belt alone. Each draw call has real CPU-side overhead (state validation, driver dispatch) independent of how few triangles it draws, so thousands of tiny draw calls is slow in a way that has nothing to do with total triangle count.

## The mechanism

`glDrawElementsInstanced` draws N copies of the *same* mesh in a *single* call, reading a different transform for each copy from a second vertex buffer that advances once per instance instead of once per vertex.

Concretely (`src/rendering/Mesh.h/.cpp`):

1. `Mesh::setInstanceTransforms(matrices)` uploads a `std::vector<glm::mat4>` to a second VBO.
2. A `mat4` isn't one GL vertex attribute — GL attributes cap out at 4 floats (a `vec4`) — so it's uploaded as **four consecutive `vec4` attributes**, locations 3-6, immediately after the mesh's own `position`/`normal`/`uv` at locations 0-2 (see `shaders/scene.vert`).
3. `glVertexAttribDivisor(location, 1)` on each of those four attributes is the actual instancing switch: divisor 0 (the default, used by locations 0-2) means "advance this attribute once per **vertex**"; divisor 1 means "advance once per **instance**." That one call is the entire difference between "every rock looks identical and overlaps at the origin" and "each rock has its own position/scale."
4. `Mesh::drawInstanced()` calls `glDrawElementsInstanced(mode, indexCount, ..., instanceCount)` — one call, GPU-side loop over all instances.

## Shader side

`shaders/scene.vert` gained a `uniform int useInstancing` and the four extra attribute inputs:

```glsl
mat4 effectiveModel = useInstancing != 0
    ? mat4(aInstanceModelCol0, aInstanceModelCol1, aInstanceModelCol2, aInstanceModelCol3)
    : model;
```

`Renderer::submit` (the existing per-object path) sets `useInstancing = 0` and uploads a single `model` uniform, unchanged. `Renderer::submitInstanced` sets `useInstancing = 1`. The per-instance attributes supply each world matrix, and the `model` uniform now carries only the floating-origin shift `translate(-cameraPosition)`, so the shader computes `model * instanceMatrix`. Same shader, same fragment stage, both paths — CLAUDE.md's existing rule ("any new mesh format needs a shader update in lockstep") applied to this format extension too.

## Object-graph side

`InstancedField` (`src/scene/InstancedField.h/.cpp`) is a minimal `SceneObject` override: its `render()` calls `Renderer::submitInstanced` instead of `submit`, and ignores its own `transform()` entirely — a belt has no single meaningful position, every instance's final world position is already baked into the matrices passed to `setInstanceTransforms` at construction time.

## Placement math (shared by all three fields)

- **Flat belt** (asteroid, Kuiper): points in an annulus, sampled by `radius = innerRadius + (outerRadius - innerRadius) * sqrt(random01)`. The `sqrt` matters — sampling radius linearly bunches points near the inner edge, because a thin ring near the center covers far less *area* than an equally-thin ring near the outside; `sqrt` corrects for that, spreading points evenly by area. A small independent Y jitter gives the belt a bit of vertical thickness instead of a mathematically perfect plane.
- **Spherical shell** (Oort cloud): same `cos(phi)`-uniform angular sampling as `StarfieldGenerator` (see `starfield.md`), plus a radius sampled by `cbrt` instead of `sqrt` — the volume-element correction for a *3D* shell instead of a 2D annulus.
- **Per-instance scale**: each instance gets an independently randomized non-uniform scale (different factor per X/Y/Z axis). A perfect sphere stretched unevenly on each axis no longer reads as a perfect sphere — a cheap stand-in for "irregular rock shape" that costs zero extra geometry.

All three fields use a fixed seed (`std::mt19937`) per field, so the belts are deterministic — same reasoning as this project's other generated content.

## Verification

- `[SCENE] <field>: N instances, 1 draw call` startup log line, once per field.
- Visual: fly through a belt and confirm many small distinctly-positioned/shaped rocks, not one blob or thousands of overlapping copies at a single point.
