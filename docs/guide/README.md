# Voyager 2 Explorer: learning guide

This guide explains the whole project from first principles, in the order you need it. Every formula is the one the code actually uses, and every chapter points to the exact files and functions, so you can read the guide and the code side by side.

![Voyager 2 six hours before Jupiter closest approach](../objects/images/runtime/03_jupiter_approach.jpg)

## Reading order

| # | Chapter | You will learn |
| --- | --- | --- |
| 1 | [How a frame is drawn](01-opengl-pipeline.md) | Window, loop, VBO/VAO/EBO, shaders, uniforms, draw calls, depth, culling |
| 2 | [Building geometry from scratch](02-geometry.md) | How every vertex, index and triangle is computed: sphere, box, cylinder, dish, ring, circle, stars, trusses |
| 3 | [Transforms, cameras and the scene graph](03-transforms-and-cameras.md) | TRS matrices, parent/child, floating origin, view/projection, logarithmic depth, camera rigs |
| 4 | [Textures](04-textures.md) | Decoding, uploading, mipmaps, UV mapping, seams, texture atlases, texture arrays, derived normal and specular maps |
| 5 | [Lighting](05-lighting.md) | Ambient, diffuse, specular; Phong and Blinn-Phong; directional, point and spot lights; attenuation; materials |
| 6 | [Shading techniques](06-shading-techniques.md) | Flat, Gouraud, Phong, Blinn-Phong and toon, where each is computed and why they look different |
| 7 | [Ray tracing](07-ray-tracing.md) | Rays, ray-sphere, ray-plane, shadow rays, soft shadows, the Whitted view, reflections, BVH, Moller-Trumbore |
| 8 | [The Voyager 2 spacecraft](08-voyager.md) | How the 18-component model is built, textured, lit, ray-traced and inspected |
| 9 | [Data, time and the mission](09-mission-data.md) | Horizons ephemeris, Hermite interpolation, the simulation clock, flyby clearance, CSV catalogs |
| 10 | [How to change things](10-how-to-change-things.md) | Step-by-step recipes for the changes you are most likely to be asked for |

## The map of the code

```text
Main.cpp                     constructs Application, calls run()
src/core/                    application-level systems
  Application                composition root: builds the scene, routes keys, runs the loop
  Window, Input, Time        GLFW window/context, keyboard+mouse state, frame timing
  CameraController           which camera rig is active and what it looks at (+ mouse picking)
  LightingController         the light rig and the lighting/shading/shadow switches
  CaptureTour                scripted screenshots (--capture ...)
src/rendering/               everything that talks to OpenGL
  Vertex, MeshData           CPU geometry format (8 floats per vertex)
  *Generator                 procedural geometry: UvSphere, Box, Cylinder, ParabolicDish, Ring, Circle, Starfield
  Mesh                       uploads MeshData into VAO/VBO/EBO and draws it
  Material, MaterialLibrary  how a surface looks and responds to light
  Texture2D, SurfaceMaps     image decode/upload; normal and specular maps derived on the CPU
  ShaderProgram              loads shaders/*.vert|frag with #include support
  Renderer                   the raster pass: camera-relative matrices, lights, shadows, translucency
  Lighting, LightingUniforms light/shading state and its upload to shaders
  RayTraceScene, RayTracer   analytic spheres/rings and the full-screen ray-traced view
  TriangleBvh                Voyager's triangles in a bounding volume hierarchy for ray tracing
  TextRenderer, BitmapFont   screen text from a hand-written 5x7 font
src/scene/                   what exists in the world (no OpenGL calls)
  SceneObject, Scene         the scene graph; named root groups
  CelestialBody, SolarSystem bodies, moons, spin, orbits, ring registry
  SolarSystemBuilder         catalog rows -> bodies and rings
  EnvironmentBuilder, Comet  stars, orbit guides, heliosphere, belts, comet
  Voyager2, VoyagerModelBuilder  the spacecraft: flight model and procedural geometry
  MissionEphemeris, MissionController, SimulationClock, Trajectory  NASA/JPL data and time
  ScaleManager               every real-to-render scale mapping
src/ui/HudOverlay            labels, telemetry panel, captions, help
shaders/                     scene.vert/.frag, lighting.glsl, raytrace*.glsl/.frag, hud.*
assets/                      textures, Horizons tables, CSV catalogs (bodies, rings, bookmarks)
docs/objects/                one reference page per object (the "encyclopedia" to this guide's "textbook")
```

## How to verify anything you change

```powershell
.\scripts\build.ps1                              # compile (Debug x64)
.\scripts\run.ps1 -NoBuild                        # run and look
.\scripts\verify_scene_layout.ps1                 # scale, data and flyby checks
.\scripts\verify_navigation_and_motion.ps1        # architecture contracts
x64\Debug\Voyager-2.exe --capture-shading shots   # screenshot every shading technique and light
```

The capture options (`--capture`, `--capture-bodies`, `--capture-shading`, `--capture-raytrace`, `--capture-voyager`) each write a folder of BMP screenshots and exit. Every image in these docs came from them.
