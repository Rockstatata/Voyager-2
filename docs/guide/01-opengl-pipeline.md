# 1. How a frame is drawn

[Guide index](README.md) · next: [Building geometry](02-geometry.md)

This chapter follows one frame from `main()` to pixels. Once you know this path, every other chapter just fills in one of its boxes.

## 1.1 Program start

`Main.cpp` does three things: construct `Application`, call `initialize()`, call `run()`. `Application::initialize()` (src/core/Application.cpp) then:

1. **`Window`** creates the GLFW window and an **OpenGL 3.3 Core** context, then loads every GL function pointer with GLAD. Only `Window` touches GLFW's lifetime.
2. **`Renderer::initialize()`** compiles `shaders/scene.vert` + `shaders/scene.frag` into one program.
3. **`RayTracer::initialize()`** compiles `shaders/raytrace.vert` + `shaders/raytrace.frag`.
4. The **scene** is built: `SolarSystemBuilder` reads `assets/data/celestial_bodies.csv` and makes the bodies. `EnvironmentBuilder` makes the stars, orbit guides and belts. `VoyagerModelBuilder` makes the spacecraft.

## 1.2 The loop

```text
while window open:
    Time.tick()                 -> dt (seconds since last frame)
    Input.poll()                -> which keys are held / pressed this frame
    Application.handleInput()   -> camera, lighting and mission keys
    Application.update(dt)      -> clock advances, bodies & Voyager move, camera follows
    Application.render()        -> draw everything (must not change simulation state)
    Window.swapBuffers()        -> show the finished image (vsync)
```

The rule **input, then update(dt), then render** is strict. Rendering only reads the scene, so a screenshot or a second view can never change where a planet is.

## 1.3 The GPU's view of the world: buffers

The GPU cannot read your C++ `std::vector`. Geometry has to be copied into GPU memory, and the GPU has to be told how to read it. Three OpenGL objects do this. Each one has an RAII wrapper in the repository root: `VBO.h`, `EBO.h` and `VAO.h`.

| Object | What it holds | Wrapper |
| --- | --- | --- |
| **VBO**: vertex buffer object | The raw bytes of every vertex | `VBO` |
| **EBO**: element buffer object | A list of vertex *indices*, three per triangle | `EBO` |
| **VAO**: vertex array object | The *recipe*: which VBO, which byte offsets, which shader inputs, which EBO | `VAO` |

### The vertex format

Every vertex in the project is the same struct (`src/rendering/Vertex.h`):

```cpp
struct Vertex
{
    glm::vec3 position;   // 3 floats: where the point is (model space)
    glm::vec3 normal;     // 3 floats: which way the surface faces there (unit length)
    glm::vec2 texCoord;   // 2 floats: where in the texture image this point samples (u, v)
};
static_assert(sizeof(Vertex) == sizeof(float) * 8);   // 32 bytes, no padding
```

In memory, a buffer of vertices looks like this (one row = one vertex = 32 bytes):

```text
byte offset: 0        12       24     32
             | px py pz | nx ny nz | u v |   vertex 0
             | px py pz | nx ny nz | u v |   vertex 1
             ...
```

### Uploading: `Mesh`

`Mesh` (src/rendering/Mesh.cpp) turns CPU `MeshData` into GPU buffers:

```cpp
Mesh::Mesh(const MeshData& data, PrimitiveMode mode)
    : m_vao(),
      m_vbo(data.vertices.data(), data.vertices.size() * sizeof(Vertex)),      // glBufferData(GL_ARRAY_BUFFER, ...)
      m_ebo(data.indices.data(),  data.indices.size()  * sizeof(uint32_t)),    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, ...)
      ...
{
    m_vao.Bind();
    m_ebo.Bind();          // the EBO binding is *stored inside* the VAO
    const GLsizeiptr stride = sizeof(Vertex);                              // 32: jump to next vertex
    m_vao.LinkAttrib(m_vbo, 0, 3, GL_FLOAT, stride, offsetof(Vertex, position)); // location 0, 3 floats at +0
    m_vao.LinkAttrib(m_vbo, 1, 3, GL_FLOAT, stride, offsetof(Vertex, normal));   // location 1, 3 floats at +12
    m_vao.LinkAttrib(m_vbo, 2, 2, GL_FLOAT, stride, offsetof(Vertex, texCoord)); // location 2, 2 floats at +24
    m_vao.Unbind();        // unbind the VAO FIRST, then the EBO, or the VAO forgets it
    ...
}
```

`LinkAttrib` wraps `glVertexAttribPointer(location, count, type, GL_FALSE, stride, offset)` plus `glEnableVertexAttribArray(location)`. The *location* numbers must match the vertex shader:

```glsl
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
```

That is why `CLAUDE.md` says `Vertex`, `Mesh` and `scene.vert` must change together.

### Drawing

```cpp
void Mesh::draw() const
{
    m_vao.Bind();
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    m_vao.Unbind();
}
```

`glDrawElements` reads the indices three at a time. Each index picks a vertex from the VBO, and every three picked vertices make one triangle. Indices let neighbouring triangles *share* vertices. A 64×32 sphere has 2,145 vertices but 3,968 triangles, and without indices it would need 11,904 vertices.

The same mesh can also be drawn as `GL_LINES`, `GL_LINE_STRIP`, `GL_LINE_LOOP` or `GL_POINTS`: see `PrimitiveMode`. Orbit guides are line loops and stars are points.

### Instancing (belts)

The asteroid belt, Kuiper belt and Oort cloud contain thousands of rocks drawn in **one** call. `Mesh::setInstanceTransforms` uploads one `mat4` per rock into a second VBO. It feeds shader locations 3–6, one `vec4` column each, with `glVertexAttribDivisor(location, 1)`, meaning "advance once per *instance*, not per vertex". Then `glDrawElementsInstanced` draws N copies. See [instancing.md](../objects/instancing.md).

## 1.4 Shaders

A **shader** is a small program that runs on the GPU. OpenGL 3.3 needs two:

- The **vertex shader** runs once per vertex. It outputs `gl_Position`, the vertex's position in *clip space* (chapter 3). It can also pass values on to the fragment shader.
- The **fragment shader** runs once per pixel that a triangle covers (a "fragment"). It outputs the pixel's colour. Values passed from the vertex shader arrive **interpolated** across the triangle, as a barycentric blend of the three vertices.

`ShaderProgram` (src/rendering/ShaderProgram.cpp) loads both files and adds one feature GLSL does not have: `#include "file.glsl"`. It inlines the file before compiling. So `lighting.glsl` (the light model) and `raytrace.glsl` (ray-sphere and shadow code) are written once and shared by the raster shaders and the ray-tracing shader. Compile and link errors are printed with the `[SHADER]` prefix.

### Uniforms

A **uniform** is a value that stays the same for a whole draw call: a matrix, a colour, a light. The CPU sets it with `glUniform*` before drawing. `ShaderProgram::uniform(name)` caches each `glGetUniformLocation` result so string lookups happen once.

Per frame (`Renderer::beginFrame`): `view`, `proj`, `logDepthCoefficient`, all lights, the ray-trace scene and texture unit numbers.
Per draw (`Renderer::submit` + `applyMaterial`): `model`, `baseColor`, `useTexture`, `shadingModel`, `specularStrength`, `specularPower`, `opacity`, `uvTransform`, `selfShadowing`, `useNormalMap`, `useSpecularMap`, `normalStrength`.

## 1.5 Fixed-function state set each frame

`Renderer::beginFrame`:

```cpp
glEnable(GL_DEPTH_TEST);  glDepthFunc(GL_LEQUAL);   // nearer fragments win
glEnable(GL_CULL_FACE);   glCullFace(GL_BACK);   glFrontFace(GL_CCW);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
```

- **Depth test.** Every pixel stores the depth of the nearest surface drawn so far. A new fragment is kept only if it is nearer (or equal). This is how Jupiter hides the moon behind it, whatever order they are drawn in.
- **Back-face culling.** A triangle whose corners appear *counter-clockwise* on screen is a **front face**. Clockwise means you are looking at its back, and it is skipped. On a closed shape, the back faces are always hidden behind front faces, so skipping them halves the work. **This is why every generator in chapter 2 is careful to wind triangles counter-clockwise when seen from outside.** If you get it backwards, the object turns inside out: you see its far inside wall.
- **Blending** is off for opaque geometry. Translucent geometry (rings, glows) is *deferred*: `submit` stores it, and `Renderer::endFrame` draws it last. It uses `glEnable(GL_BLEND)` and `glDepthMask(GL_FALSE)` (tested against depth but not writing it), so translucent sheets never hide what is behind them.

## 1.6 One frame in this project, in order

`Application::render()`:

1. `Renderer::beginFrame`: clear, set frame uniforms.
2. Background star layers (`submitBackground`, depth writes off, always behind).
3. The scene graph: `Scene::render` walks every root group and calls each object's `render`, which calls `Renderer::submit(mesh, material, worldMatrix)`.
4. If the ray-traced view is on (F9), `RayTracer::render` draws the traced worlds over the frame (chapter 7). The bodies and spacecraft groups are hidden from the raster pass for that frame.
5. `Renderer::endFrame`: the deferred translucent and glow draws.
6. HUD: text and labels, drawn in screen space by `TextRenderer` with `shaders/hud.*`.

Next: where the vertices themselves come from.
