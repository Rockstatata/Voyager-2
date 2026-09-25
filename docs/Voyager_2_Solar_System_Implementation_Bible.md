# Voyager 2 Solar-System Explorer
## Master Implementation Bible — Visual Studio + C++ + OpenGL + GLAD + GLFW

**Document role:** Living technical specification and implementation checklist  
**Project type:** Interactive 3D solar-system / Voyager 2 exploration application  
**Primary environment:** Microsoft Visual Studio on Windows  
**Graphics API:** Modern OpenGL core profile  
**Window/input/context:** GLFW  
**OpenGL loader:** GLAD  
**Mathematics:** GLM recommended  
**Primary language:** C++17 or newer  
**Document version:** 1.0  
**Status:** Foundation planning → implementation

---

# Table of Contents

1. [Purpose of This Bible](#1-purpose-of-this-bible)
2. [Project Vision](#2-project-vision)
3. [Non-Negotiable Requirements](#3-non-negotiable-requirements)
4. [Scope and Evaluation Priorities](#4-scope-and-evaluation-priorities)
5. [Technology Stack](#5-technology-stack)
6. [Development Rules](#6-development-rules)
7. [Visual Studio Project Setup](#7-visual-studio-project-setup)
8. [Recommended Repository Structure](#8-recommended-repository-structure)
9. [High-Level Architecture](#9-high-level-architecture)
10. [Main Application Loop](#10-main-application-loop)
11. [Core Math and Coordinate Conventions](#11-core-math-and-coordinate-conventions)
12. [Transform System](#12-transform-system)
13. [Scene Object and Scene Graph](#13-scene-object-and-scene-graph)
14. [Rendering Architecture](#14-rendering-architecture)
15. [Mesh and Geometry System](#15-mesh-and-geometry-system)
16. [Asset and Model Loading](#16-asset-and-model-loading)
17. [Material and Shader Readiness](#17-material-and-shader-readiness)
18. [Solar-System Data Model](#18-solar-system-data-model)
19. [Scale Problem and Scale Manager](#19-scale-problem-and-scale-manager)
20. [Floating-Origin / Large-World Precision](#20-floating-origin--large-world-precision)
21. [Simulation Clock](#21-simulation-clock)
22. [Celestial-Body Architecture](#22-celestial-body-architecture)
23. [Sun](#23-sun)
24. [Planets](#24-planets)
25. [Moons](#25-moons)
26. [Planetary Rings](#26-planetary-rings)
27. [Asteroid Belt](#27-asteroid-belt)
28. [Kuiper Belt and Outer-System Objects](#28-kuiper-belt-and-outer-system-objects)
29. [Heliosphere and Interstellar-Space Visualization](#29-heliosphere-and-interstellar-space-visualization)
30. [Background Stars](#30-background-stars)
31. [Voyager 2 Spacecraft Model](#31-voyager-2-spacecraft-model)
32. [Voyager Dual-Mode Architecture](#32-voyager-dual-mode-architecture)
33. [Historical Trajectory Mode](#33-historical-trajectory-mode)
34. [Manual Flight Mode](#34-manual-flight-mode)
35. [Mode Switching](#35-mode-switching)
36. [Input System](#36-input-system)
37. [Camera System](#37-camera-system)
38. [Trajectory Rendering](#38-trajectory-rendering)
39. [HUD, Labels, and Telemetry](#39-hud-labels-and-telemetry)
40. [Object Selection and Focus](#40-object-selection-and-focus)
41. [Performance Strategy](#41-performance-strategy)
42. [Error Handling and Debugging](#42-error-handling-and-debugging)
43. [Testing Strategy](#43-testing-strategy)
44. [Data and Configuration Files](#44-data-and-configuration-files)
45. [Implementation Phases](#45-implementation-phases)
46. [Next-Lab 50% Evaluation Definition of Done](#46-next-lab-50-evaluation-definition-of-done)
47. [Later Lighting and Shader Integration](#47-later-lighting-and-shader-integration)
48. [Presentation / Demonstration Flow](#48-presentation--demonstration-flow)
49. [Coding Standards](#49-coding-standards)
50. [Git and Change Discipline](#50-git-and-change-discipline)
51. [Common Failure Modes to Avoid](#51-common-failure-modes-to-avoid)
52. [Recommended Class Interfaces](#52-recommended-class-interfaces)
53. [First Concrete Implementation Sprint](#53-first-concrete-implementation-sprint)
54. [Backlog](#54-backlog)
55. [Decision Log](#55-decision-log)
56. [Implementation Session Log Template](#56-implementation-session-log-template)
57. [Reference Sources](#57-reference-sources)

---

# 1. Purpose of This Bible

This file is the project's **single technical source of truth**.

Every important implementation decision should either:

- already exist here,
- be added here before or during implementation,
- or be recorded in the decision log after we discover that the instructor's starter project requires a different approach.

The goal is to prevent the project from becoming a collection of unrelated OpenGL experiments.

We are building a coherent application with:

- a reusable rendering foundation,
- a complete solar-system scene,
- physically meaningful astronomical data,
- visual scaling suitable for human viewing,
- a detailed Voyager 2 spacecraft,
- an actual/historical Voyager 2 trajectory,
- a fully pilot-controllable Voyager 2 mode,
- multiple camera modes,
- trajectory visualization,
- support for the instructor's later lighting and shader files,
- and enough modularity that new objects and effects can be added without rewriting the entire program.

## Golden rule

> **We do not implement a planet, moon, spacecraft, orbit, camera, or shader as a one-off special case unless there is a strong reason.**

Build systems, then instantiate objects through those systems.

---

# 2. Project Vision

The final project should feel like a small **Voyager 2 Solar-System Explorer**.

The user should be able to:

1. Enter a 3D solar-system scene.
2. View the Sun, planets, important moons, rings, belts, Voyager 2, and large-scale solar boundaries.
3. Move through time.
4. See Voyager 2 follow its historical mission trajectory.
5. Observe the Jupiter, Saturn, Uranus, and Neptune flybys.
6. Switch to manual control.
7. Pilot Voyager with full 3D orientation and translation.
8. Switch between free, chase, forward, and object-focus cameras.
9. Show or hide orbit and trajectory guides.
10. Eventually travel beyond the planetary region toward the heliosphere and interstellar space.
11. Use the instructor-provided lighting and shader system without restructuring the whole scene.

The application is therefore both:

- an **astronomical visualization**, and
- an **interactive spacecraft navigation experience**.

---

# 3. Non-Negotiable Requirements

These requirements should drive every architecture decision.

## R1 — One Voyager, two flight modes

There must be **one Voyager 2 scene object** controlled by either:

- `RealTrajectoryController`
- `ManualFlightController`

Do **not** create `VoyagerHistorical` and `VoyagerManual` as separate spacecraft.

---

## R2 — Historical planetary positions and historical Voyager position use the same simulation time

If the simulation date is the Jupiter flyby, both:

- Jupiter's position
- Voyager's position

must be evaluated for that same simulated date.

Otherwise a historically correct spacecraft trajectory and a separately animated solar system will not meet correctly.

---

## R3 — Separate physical data from render data

Never permanently distort the physical data just to make it visible.

Keep:

- real radius,
- real distance,
- real or approximate physical position,

separate from:

- render radius,
- render distance,
- display exaggeration.

---

## R4 — Large-world precision must be planned from the beginning

Solar-system and interstellar distances are too large for a naive all-`float` world.

Use:

- `double` / `glm::dvec3` for simulation-space coordinates,
- camera-relative `float` / `glm::vec3` for GPU rendering.

---

## R5 — Every visible object must be material/shader-ready

Even if next lab uses basic rendering, every renderable object should ultimately pass through a common rendering interface.

Example:

```cpp
renderer.draw(mesh, material, modelMatrix);
```

The instructor's later lighting/shader code should be integrated into `Renderer`, `Material`, and shader classes, not copied into every planet class.

---

## R6 — Objects must be data-driven where practical

Earth should mainly differ from Mars because of data:

```text
radius
rotation
orbit
texture/material
axial tilt
parent
```

not because Earth has an entirely different renderer.

---

## R7 — The next lab prioritizes complete object readiness

Before spending major time on advanced lighting:

- all major object categories should exist,
- their transforms should work,
- their hierarchy should work,
- basic materials should render,
- Voyager should exist,
- both Voyager control modes should have a working foundation.

---

# 4. Scope and Evaluation Priorities

## Immediate priority: next lab

The next lab carries approximately **50% of the relevant marking**, with emphasis on making the project objects ready.

Therefore the immediate definition of success is:

> The entire scene structure exists and is demonstrable using basic rendering, even if final lighting/shaders are not yet integrated.

## Immediate visual objects

### Stellar

- Sun

### Planets

- Mercury
- Venus
- Earth
- Mars
- Jupiter
- Saturn
- Uranus
- Neptune

### Outer / dwarf object support

- Pluto is recommended for completeness of solar-system exploration even though Voyager 2 did not encounter it.
- Additional dwarf objects are stretch goals.

### Important moons

At minimum prioritize moons that are highly recognizable or connected to Voyager 2 encounters.

#### Earth
- Moon

#### Jupiter
- Io
- Europa
- Ganymede
- Callisto

#### Saturn
- Titan
- optionally Rhea, Iapetus, Dione, Tethys

#### Uranus
- Miranda
- Ariel
- Umbriel
- Titania
- Oberon

#### Neptune
- Triton

### Ring systems

Architecture should support rings for:

- Jupiter
- Saturn
- Uranus
- Neptune

Saturn's should be the most visually developed.

### Distributed small objects

- Asteroid belt
- Kuiper-belt representation

### Large-scale / abstract boundaries

- heliosphere representation
- termination shock marker or shell
- heliopause marker or shell
- interstellar region / background

### Spacecraft

- Voyager 2
- distinguishable major components

### Guides

- planetary orbit lines
- Voyager trajectory line
- object labels / markers

---

# 5. Technology Stack

## Required core

| Component | Role |
|---|---|
| C++ | Application and simulation code |
| OpenGL | Rendering API |
| GLFW | Window, OpenGL context, input/events |
| GLAD | Loads OpenGL function pointers |
| Visual Studio | Windows IDE/compiler/debugger |

## Recommended support

| Component | Role |
|---|---|
| GLM | vectors, matrices, quaternions, transforms |
| stb_image | simple texture loading if not already provided |
| tinyobjloader or Assimp | model loading, only if the starter project needs it |
| nlohmann/json | data-driven body/config loading, optional but useful |

### Dependency discipline

Do **not** add a dependency merely because it exists.

First inspect the instructor's starter project. Reuse its:

- shader loader,
- model loader,
- texture loader,
- file structure,
- helper classes,

when they are suitable.

---

# 6. Development Rules

## Rule 1 — Preserve the instructor starter project first

Before restructuring:

1. Build the original project.
2. Confirm the cube renders.
3. Create a clean Git commit/tag.
4. Record current controls and dependencies.
5. Only then start refactoring.

Suggested tag:

```text
starter-project-working
```

---

## Rule 2 — Never break the main branch for multiple days

Each implementation increment should end in a buildable state whenever possible.

---

## Rule 3 — Refactor before duplication

If the same logic appears for Earth and Mars, ask whether it belongs in:

- `CelestialBody`
- `Renderer`
- `Transform`
- `Orbit`
- `Material`

before copying it.

---

## Rule 4 — Separate Update from Render

Objects should not modify simulation state inside their draw function.

Preferred pattern:

```cpp
app.update(deltaTime);
app.render();
```

---

## Rule 5 — OpenGL calls belong mainly in rendering/resource classes

Avoid scattering:

```cpp
glBindVertexArray(...)
glUseProgram(...)
glBindTexture(...)
```

through unrelated simulation classes.

---

## Rule 6 — Physical simulation does not depend on the camera

Moving the camera must never change a planet's physical coordinate.

---

# 7. Visual Studio Project Setup

We will adapt this to the supplied project rather than blindly replacing its configuration.

## 7.1 Platform

Use:

```text
x64
```

for both:

- Debug
- Release

Do not accidentally configure dependencies only for `Debug | Win32`.

---

## 7.2 C++ standard

Minimum recommended:

```text
C++17
```

Visual Studio:

```text
Project Properties
→ C/C++
→ Language
→ C++ Language Standard
→ ISO C++17 or newer
```

---

## 7.3 OpenGL context version

**Use the version already required by the instructor's starter project.**

If starting from nothing, OpenGL 3.3 Core is a conservative educational baseline.

Do not arbitrarily request OpenGL 4.6 if lab PCs may not support it.

Example context hints:

```cpp
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
```

If the provided code uses a later version, preserve that version.

---

## 7.4 Include order

GLAD should provide OpenGL declarations.

Typical order:

```cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>
```

Do not manually include legacy `GL/gl.h` alongside a generated GLAD setup unless the starter project explicitly requires it.

---

## 7.5 GLFW initialization pattern

Conceptually:

```cpp
if (!glfwInit())
{
    // Log error and exit.
}

GLFWwindow* window =
    glfwCreateWindow(width, height, "Voyager 2 Explorer", nullptr, nullptr);

if (!window)
{
    glfwTerminate();
    // Log error and exit.
}

glfwMakeContextCurrent(window);
```

---

## 7.6 GLAD initialization

The exact GLAD call depends on which GLAD generation/version the instructor project contains.

For a common GLFW + GLAD configuration, initialization is performed after the context is current.

Example style often seen in GLAD 1-based starter projects:

```cpp
if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
{
    // Failed to initialize GLAD.
}
```

Some newer GLAD-generated projects expose different loader entry points.

**Use the API matching the generated GLAD files already in the project.**

---

## 7.7 Visual Studio include/library directories

Possible local layout:

```text
external/
├── glad/
│   ├── include/
│   └── src/
├── glfw/
│   ├── include/
│   └── lib/
└── glm/
    └── glm/
```

Typical configuration:

```text
C/C++ → General → Additional Include Directories
```

Add relevant include directories.

For GLFW static library:

```text
Linker → General → Additional Library Directories
```

and:

```text
Linker → Input → Additional Dependencies
```

typically includes:

```text
glfw3.lib
opengl32.lib
```

The exact GLFW library name depends on how it is supplied.

---

## 7.8 GLAD source

If GLAD ships with a generated C source file such as:

```text
glad.c
```

it must be compiled as part of the Visual Studio project.

Do not only add the header.

---

## 7.9 Recommended warnings

Use strong compiler warnings when possible.

```text
/W4
```

Resolve warnings as implementation proceeds instead of allowing hundreds to accumulate.

---

## 7.10 Working directory

Assets often fail because Visual Studio runs the executable from a different directory.

Choose one consistent approach:

### Option A
Set the debugging working directory to the project root.

### Option B
Implement an asset path manager.

Preferred eventual structure:

```cpp
AssetManager::getPath("textures/earth.jpg");
```

Avoid fragile paths like:

```cpp
"..\\..\\..\\textures\\earth.jpg"
```

throughout the program.

---

# 8. Recommended Repository Structure

Adapt names to the starter project if it already has useful conventions.

```text
VoyagerExplorer/
│
├── VoyagerExplorer.sln
├── README.md
├── IMPLEMENTATION_BIBLE.md
│
├── src/
│   │
│   ├── main.cpp
│   │
│   ├── core/
│   │   ├── Application.h
│   │   ├── Application.cpp
│   │   ├── Window.h
│   │   ├── Window.cpp
│   │   ├── Input.h
│   │   ├── Input.cpp
│   │   ├── Time.h
│   │   └── Time.cpp
│   │
│   ├── rendering/
│   │   ├── Renderer.h
│   │   ├── Renderer.cpp
│   │   ├── Mesh.h
│   │   ├── Mesh.cpp
│   │   ├── Model.h
│   │   ├── Model.cpp
│   │   ├── Shader.h
│   │   ├── Shader.cpp
│   │   ├── Material.h
│   │   ├── Texture.h
│   │   ├── Texture.cpp
│   │   ├── Camera.h
│   │   └── Camera.cpp
│   │
│   ├── scene/
│   │   ├── Transform.h
│   │   ├── SceneObject.h
│   │   ├── SceneObject.cpp
│   │   ├── Scene.h
│   │   └── Scene.cpp
│   │
│   ├── solar/
│   │   ├── SolarSystem.h
│   │   ├── SolarSystem.cpp
│   │   ├── CelestialBody.h
│   │   ├── CelestialBody.cpp
│   │   ├── Orbit.h
│   │   ├── Orbit.cpp
│   │   ├── RingSystem.h
│   │   ├── RingSystem.cpp
│   │   ├── AsteroidBelt.h
│   │   ├── AsteroidBelt.cpp
│   │   ├── KuiperBelt.h
│   │   └── KuiperBelt.cpp
│   │
│   ├── voyager/
│   │   ├── Voyager2.h
│   │   ├── Voyager2.cpp
│   │   ├── VoyagerController.h
│   │   ├── RealTrajectoryController.h
│   │   ├── RealTrajectoryController.cpp
│   │   ├── ManualFlightController.h
│   │   ├── ManualFlightController.cpp
│   │   ├── Trajectory.h
│   │   └── Trajectory.cpp
│   │
│   ├── simulation/
│   │   ├── SimulationClock.h
│   │   ├── SimulationClock.cpp
│   │   ├── ScaleManager.h
│   │   ├── ScaleManager.cpp
│   │   ├── CoordinateSystem.h
│   │   ├── EphemerisProvider.h
│   │   └── EphemerisProvider.cpp
│   │
│   └── ui/
│       ├── HUD.h
│       ├── HUD.cpp
│       ├── Labels.h
│       └── Labels.cpp
│
├── assets/
│   ├── models/
│   │   ├── voyager/
│   │   ├── planets/
│   │   ├── moons/
│   │   └── asteroids/
│   ├── textures/
│   │   ├── planets/
│   │   ├── moons/
│   │   ├── rings/
│   │   └── misc/
│   ├── shaders/
│   ├── data/
│   │   ├── celestial_bodies.json
│   │   └── mission_bookmarks.json
│   └── trajectory/
│       └── voyager2_trajectory.csv
│
├── external/
└── tools/
    └── trajectory_preprocess/
```

---

# 9. High-Level Architecture

```text
Application
│
├── Window / GLFW
├── Input
├── SimulationClock
├── Scene
│   │
│   ├── SolarSystem
│   │   ├── Sun
│   │   ├── Planets
│   │   ├── Moons
│   │   ├── Rings
│   │   ├── AsteroidBelt
│   │   ├── KuiperBelt
│   │   └── Heliosphere
│   │
│   └── Voyager2
│       ├── Spacecraft hierarchy
│       └── Active controller
│           ├── Historical
│           └── Manual
│
├── CameraSystem
├── Renderer
├── AssetManager
└── UI / HUD
```

## Responsibility rule

### Application
Owns lifecycle.

### Scene
Owns objects.

### Simulation
Calculates state.

### Renderer
Draws state.

### Controllers
Change the state of a controllable object.

### Camera
Determines view.

### UI
Displays information.

---

# 10. Main Application Loop

Target conceptual loop:

```cpp
while (!glfwWindowShouldClose(window))
{
    const double deltaTime = time.update();

    input.beginFrame();
    glfwPollEvents();

    simulationClock.update(deltaTime);

    scene.update(deltaTime);
    cameraSystem.update(deltaTime);

    renderer.beginFrame(cameraSystem.activeCamera());
    scene.render(renderer);
    ui.render(renderer);
    renderer.endFrame();

    glfwSwapBuffers(window);
}
```

The exact order can change based on the supplied code.

## Important

Avoid a massive `main.cpp`.

Target eventually:

```cpp
int main()
{
    Application app;
    return app.run();
}
```

---

# 11. Core Math and Coordinate Conventions

We must choose conventions once.

Recommended:

```text
Right-handed coordinate system
+X = right
+Y = up
-Z = forward for camera conventions commonly used in OpenGL examples
```

For astronomical coordinates, we may use an imported coordinate frame and convert it once into the render convention.

## Unit conventions

### Physical position
Use kilometers or astronomical units, documented explicitly.

Recommended internal base:

```text
kilometers
```

for spacecraft/celestial physical position, with helpers:

```cpp
constexpr double KM_PER_AU = 149597870.7;
```

### Angles

Internally prefer radians.

```cpp
glm::radians(degrees)
```

### Time

Do not mix:

- wall-clock time,
- frame delta,
- simulation date.

Keep separate.

---

# 12. Transform System

Every scene object should have a reusable transform.

Suggested design:

```cpp
struct Transform
{
    glm::dvec3 position {0.0};
    glm::dquat rotation {1.0, 0.0, 0.0, 0.0};
    glm::dvec3 scale {1.0};

    glm::dmat4 localMatrix() const;
};
```

For GPU rendering, build a float matrix after camera-relative conversion.

## Why quaternion rotation?

Voyager manual flight requires:

- pitch,
- yaw,
- roll,
- arbitrary orientation.

Repeated Euler-angle manipulation becomes fragile.

Use:

```cpp
glm::quat
```

or:

```cpp
glm::dquat
```

for orientation.

---

# 13. Scene Object and Scene Graph

Base concept:

```cpp
class SceneObject
{
public:
    virtual ~SceneObject() = default;

    virtual void update(double dt);
    virtual void render(Renderer& renderer);

    Transform& transform();
    const Transform& transform() const;

    void addChild(std::shared_ptr<SceneObject> child);

protected:
    Transform m_transform;
    SceneObject* m_parent = nullptr;
    std::vector<std::shared_ptr<SceneObject>> m_children;
};
```

Ownership model can be adjusted after inspecting the starter project.

## Parent-child examples

```text
Earth
└── Moon
```

```text
Saturn
└── RingSystem
```

```text
Voyager2
├── Dish
├── MainBus
├── MagnetometerBoom
└── RTGBoom
```

## World transform

```text
worldTransform = parentWorldTransform × localTransform
```

Do not manually update the Moon's world coordinate from scratch if it can naturally be represented relative to Earth.

---

# 14. Rendering Architecture

The renderer should be able to draw an arbitrary mesh/material/transform combination.

Conceptually:

```cpp
class Renderer
{
public:
    void beginFrame(const Camera& camera);
    void submit(
        const Mesh& mesh,
        const Material& material,
        const glm::mat4& modelMatrix
    );
    void endFrame();
};
```

For the first lab, immediate rendering calls inside `submit` are acceptable.

Later, `submit` could batch/sort draw commands.

## Renderer owns OpenGL state policy

Examples:

- depth testing
- culling
- blending policy
- polygon mode
- shader binding
- texture binding

Object simulation classes should not manage these globally.

---

# 15. Mesh and Geometry System

## Required mesh types

1. Cube — existing starter object; retain as test geometry.
2. UV sphere — Sun, planets, moons.
3. Ring mesh — planetary rings.
4. Line mesh — orbit and trajectory lines.
5. Simple asteroid mesh / imported rock meshes.
6. Voyager model meshes.
7. Large shell/sphere geometry — heliosphere visualization if needed.

## Mesh class

```cpp
class Mesh
{
public:
    Mesh(
        const std::vector<Vertex>& vertices,
        const std::vector<unsigned int>& indices
    );

    void draw() const;

private:
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_ebo = 0;
    std::size_t m_indexCount = 0;
};
```

## Vertex

Prepare for later lighting now.

Even if current shader uses only position, make the model pipeline capable of:

```cpp
struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};
```

Optional later:

```text
tangent
bitangent
```

---

# 16. Asset and Model Loading

## Strategy

Do not assume every object needs a unique imported 3D model.

### Procedural/shared geometry

Use shared sphere mesh for:

- planets
- many moons
- Sun

### Imported model

Use imported geometry for:

- Voyager 2
- optional detailed asteroids
- special educational objects

## Model normalization

When importing Voyager:

1. Record original coordinate orientation.
2. Record original dimensions.
3. Normalize only through a model-root transform.
4. Do not destructively edit every child transform unless necessary.
5. Establish a documented "spacecraft forward" axis.

Example decision:

```text
Voyager local forward = -Z
Voyager local up      = +Y
```

Once chosen, manual-control equations must follow it consistently.

---

# 17. Material and Shader Readiness

The instructor will later provide lighting/shader files.

We must prepare now without duplicating their future work.

## Material

Concept:

```cpp
struct Material
{
    Shader* shader = nullptr;

    Texture* albedo = nullptr;
    Texture* normal = nullptr;
    Texture* specular = nullptr;

    glm::vec3 baseColor {1.0f};

    float roughness = 1.0f;
    float metallic = 0.0f;
};
```

Not all fields need to be active immediately.

## Goal

Current basic shader:

```text
Mesh + Transform + basic color/texture
```

Later:

```text
same Mesh + same Transform + improved Material + instructor Shader
```

No object rewrite.

---

# 18. Solar-System Data Model

A celestial body should primarily be data.

Suggested physical fields:

```cpp
struct CelestialBodyData
{
    std::string id;
    std::string displayName;
    std::string parentId;

    double radiusKm;
    double semiMajorAxisKm;
    double orbitalPeriodDays;
    double rotationPeriodHours;
    double axialTiltDegrees;

    std::string texturePath;
    std::string materialId;

    BodyType type;
};
```

Later orbital elements may include:

```text
eccentricity
inclination
longitude of ascending node
argument of periapsis
mean anomaly at epoch
epoch
```

## First implementation

We do not need full celestial mechanics on day one.

Progression:

```text
static positions
→ circular orbit approximation
→ orbital elements / ephemeris
→ synchronized historical positions
```

Architecture must allow this replacement.

---

# 19. Scale Problem and Scale Manager

The solar system cannot be rendered intelligibly with one naive real-world scale.

## Example problem

If the Sun and Earth-Sun distance use the same literal scale and the entire solar system fits on screen, Earth becomes extremely small.

If Earth is made clearly visible using the same scale, interplanetary distances become enormous.

Therefore use **independent visualization mapping** for:

- body radius
- body distance
- spacecraft display size

while preserving physical data.

## ScaleManager concept

```cpp
class ScaleManager
{
public:
    double distanceToRenderUnits(double distanceKm) const;
    double radiusToRenderUnits(double radiusKm) const;
    double spacecraftSizeToRenderUnits(double sizeMeters) const;

    ScaleMode mode() const;
};
```

## Scale modes

Recommended eventual modes:

```text
Educational
ApproximatePhysical
LocalEncounter
```

### Educational

- planet radii exaggerated
- distances compressed
- useful for seeing the solar system

### Approximate Physical

- closer to consistent scale
- educational demonstration of scale difficulty

### Local Encounter

- center around Voyager / selected body
- spacecraft becomes visible near a planet

---

# 20. Floating-Origin / Large-World Precision

## Problem

A 32-bit float has limited precision.

At huge solar-system coordinates, small movements can become unstable or disappear.

## Solution

Store physical world position in double precision:

```cpp
glm::dvec3 worldPositionKm;
```

Camera physical position:

```cpp
glm::dvec3 cameraWorldPositionKm;
```

Before rendering:

```cpp
glm::dvec3 relative =
    objectWorldPositionKm - cameraWorldPositionKm;
```

Then map/scale and convert:

```cpp
glm::vec3 renderPosition =
    glm::vec3(scaleManager.toRender(relative));
```

Concept:

```text
huge double-precision universe
            │
            ▼
 subtract camera/scene origin
            │
            ▼
 smaller camera-relative coordinate
            │
            ▼
 visualization scale
            │
            ▼
 float GPU transform
```

This strategy should be introduced before the interstellar phase.

---

# 21. Simulation Clock

The simulation clock is central.

```cpp
class SimulationClock
{
public:
    void update(double realDeltaSeconds);

    void setPaused(bool paused);
    void togglePause();

    void setTimeScale(double scale);

    double timeScale() const;
    double currentJulianDate() const;

    void setJulianDate(double jd);

private:
    double m_currentJulianDate = 0.0;
    double m_timeScale = 1.0;
    bool m_paused = false;
};
```

## Distinguish three time domains

### Real frame time

Used for:

- input smoothing
- camera motion
- UI animation

### Simulation time

Used for:

- planets
- moons
- historical Voyager

### Manual spacecraft integration time

Decision:

Manual flight should normally use real frame delta or a specifically chosen flight time scale.

Do not accidentally multiply pilot controls by `10000x` historical simulation speed unless that behavior is explicitly desired.

---

## Time controls

Recommended:

```text
P        Pause/unpause historical simulation
+ / =    Increase simulation time scale
-        Decrease simulation time scale
0        Reset time scale
```

Possible speed presets:

```text
1x
10x
100x
1000x
10000x
```

Mission bookmarks are better than forcing the user to wait through decades.

---

# 22. Celestial-Body Architecture

```cpp
class CelestialBody : public SceneObject
{
public:
    explicit CelestialBody(const CelestialBodyData& data);

    void update(double dt) override;
    void render(Renderer& renderer) override;

    const CelestialBodyData& data() const;

private:
    CelestialBodyData m_data;

    std::shared_ptr<Mesh> m_sphereMesh;
    std::shared_ptr<Material> m_material;
};
```

## Body responsibilities

A celestial body can know:

- its data,
- local spin,
- orbital state,
- geometry/material reference.

It should not know:

- keyboard state,
- window size,
- global OpenGL state.

---

# 23. Sun

The Sun is the natural scene root for heliocentric visualization.

Initial version:

- sphere mesh
- unique material
- static at solar-system origin
- basic rotation optional

Later:

- emissive shader
- bloom
- animated surface
- solar lighting source

## Important future compatibility

Do not make the Sun rely exclusively on the same lit material as planets.

It will likely use a special emissive material.

---

# 24. Planets

All eight planets must exist.

## Minimum properties per planet

- name
- sphere mesh
- radius data
- display scale
- texture/material
- axial tilt
- spin
- orbit/position
- label
- selection ID

## Shared sphere mesh

Generate once at sufficient resolution.

Example:

```text
64 longitudinal segments
32 latitudinal segments
```

Exact resolution should be tested against performance.

Do not create eight identical sphere VAOs if one shared mesh can be reused.

---

# 25. Moons

Moons use the same `CelestialBody` concept.

Parent relationship:

```text
Sun
└── Earth
    └── Moon
```

For outer planets:

```text
Jupiter
├── Io
├── Europa
├── Ganymede
└── Callisto
```

## Moon rendering scale

Educational mode may need stronger exaggeration than planet radius scaling.

This is acceptable as long as physical radius remains stored separately.

---

# 26. Planetary Rings

Create a generic ring system.

```cpp
struct RingData
{
    double innerRadiusKm;
    double outerRadiusKm;
    std::string texturePath;
};
```

```cpp
class RingSystem : public SceneObject
{
    // Shared annulus mesh + material.
};
```

## Geometry

Annulus:

```text
inner radius
outer radius
radial subdivisions
angular subdivisions
```

Texture UVs should support radial ring textures.

## Important

Rings inherit planet:

- position
- orientation basis

but may have their own local tilt/alignment.

---

# 27. Asteroid Belt

Never instantiate thousands of heavyweight `SceneObject`s with independent materials if avoidable.

## Preferred method

GPU instancing.

```text
1–3 asteroid meshes
        │
        ▼
instance transform buffer
        │
        ▼
hundreds/thousands of rendered rocks
```

Instance data can contain:

```text
position
scale
rotation
```

## Distribution

Use deterministic random generation.

Seed:

```cpp
std::mt19937 rng(knownSeed);
```

Benefits:

- scene looks the same every run,
- debugging is easier,
- screenshots are reproducible.

---

# 28. Kuiper Belt and Outer-System Objects

Use the same principle as the asteroid belt but with:

- wider distribution,
- greater distances,
- fewer/lower-detail instances if necessary.

Possible recognizable bodies:

- Pluto
- Charon
- optional dwarf-planet markers

Do not allow the Kuiper belt to dominate performance.

---

# 29. Heliosphere and Interstellar-Space Visualization

These objects represent abstract/large-scale regions, not solid planets.

Possible representations:

### Heliosphere
- transparent large shell
- wireframe/line shell
- labeled boundary

### Termination shock
- optional shell/marker

### Heliopause
- large boundary shell/marker

### Interstellar region
- background changes
- sparse particles
- labels

For the object-readiness lab, placeholder geometry is acceptable.

Example:

```text
large low-resolution sphere
transparent/wireframe basic material
```

Later shaders can make it visually meaningful.

---

# 30. Background Stars

Recommended techniques:

### Simple
Skybox cube.

### Alternative
Large inward-facing sphere with star texture.

### Advanced
Procedural point stars.

For current scope, a skybox or star sphere is enough.

Important:

- star background should appear infinitely distant,
- camera translation should not make the skybox visibly move.

---

# 31. Voyager 2 Spacecraft Model

Voyager is the hero object.

## Prefer hierarchical representation

```text
Voyager2Root
│
├── MainBus
├── HighGainAntenna
│   └── Dish
├── RTGBoom
│   ├── RTG1
│   ├── RTG2
│   └── RTG3
├── MagnetometerBoom
├── ScienceBoom
├── LowGainAntenna
└── InstrumentComponents
```

If the supplied model arrives as one mesh, we can still place it below `Voyager2Root`.

If it is composed of many meshes, preserve meaningful hierarchy.

## Voyager root state

```cpp
class Voyager2 : public SceneObject
{
public:
    void setMode(VoyagerMode mode);
    VoyagerMode mode() const;

    void update(double dt) override;

    glm::dvec3 physicalPositionKm() const;
    glm::dquat orientation() const;

private:
    VoyagerMode m_mode;
    std::unique_ptr<VoyagerController> m_controller;
};
```

---

# 32. Voyager Dual-Mode Architecture

```text
                       Voyager2
                          │
                          ▼
                   VoyagerController
                    /             \
                   /               \
   RealTrajectoryController   ManualFlightController
```

## Interface

```cpp
class VoyagerController
{
public:
    virtual ~VoyagerController() = default;

    virtual void onActivated(Voyager2& voyager) {}
    virtual void onDeactivated(Voyager2& voyager) {}

    virtual void update(
        Voyager2& voyager,
        double deltaTime
    ) = 0;
};
```

This avoids mode-specific code inside the render method.

---

# 33. Historical Trajectory Mode

## Objective

Voyager follows a credible historical path synchronized with the same simulation date used for planets.

NASA identifies Voyager 2's major flybys as:

- Jupiter — 1979
- Saturn — 1981
- Uranus — 1986
- Neptune — 1989

Voyager 2 later continued outward and entered interstellar space in 2018.

## Recommended accuracy path

### Stage H1 — demo waypoints

Temporary during implementation.

```text
Launch area
Jupiter encounter
Saturn encounter
Uranus encounter
Neptune encounter
outbound point
```

Interpolate between them.

This proves the controller architecture.

### Stage H2 — sampled official trajectory

Use NASA/JPL NAIF SPICE data offline to sample Voyager 2 states.

Store project-friendly data:

```csv
julian_date,x_km,y_km,z_km,vx_km_s,vy_km_s,vz_km_s
...
```

### Stage H3 — better interpolation

Between samples:

- linear position interpolation initially
- Hermite/Catmull-Rom or velocity-aware interpolation later

For scientific visualization, avoid oversmoothing in ways that visibly change encounter geometry.

---

## Trajectory lookup

```cpp
struct TrajectorySample
{
    double julianDate;
    glm::dvec3 positionKm;
    glm::dvec3 velocityKmPerSec;
};
```

```cpp
class Trajectory
{
public:
    bool loadCsv(const std::string& path);

    TrajectoryState sample(double julianDate) const;

    const std::vector<TrajectorySample>& samples() const;

private:
    std::vector<TrajectorySample> m_samples;
};
```

## Historical controller

```cpp
void RealTrajectoryController::update(
    Voyager2& voyager,
    double /*dt*/)
{
    const auto state =
        m_trajectory.sample(m_clock.currentJulianDate());

    voyager.setPhysicalPositionKm(state.positionKm);

    // Orientation can initially follow velocity.
}
```

## Orientation in historical mode

Simple first solution:

```text
forward direction = normalized velocity vector
```

Later, if actual spacecraft attitude data is available and needed, it could be incorporated separately.

This project does not need full real attitude telemetry to demonstrate historical path correctness unless explicitly required.

---

# 34. Manual Flight Mode

## Objective

User can:

- steer
- pitch
- yaw
- roll
- thrust forward/backward
- optionally strafe
- reorient in full 3D

## State

```cpp
struct FlightState
{
    glm::dvec3 positionKm;
    glm::dvec3 velocityKmPerSec;

    glm::dquat orientation;

    glm::dvec3 angularVelocity;
};
```

We can simplify angular velocity for the first lab if direct rotational control is sufficient.

---

## Recommended controls

| Key | Action |
|---|---|
| W | forward thrust |
| S | reverse/brake thrust |
| A | translate left |
| D | translate right |
| R | translate up |
| F | translate down |
| Arrow Up / Mouse | pitch up/down |
| Arrow Left/Right / Mouse | yaw |
| Q | roll left |
| E | roll right |
| Left Shift | boost |
| Space | flight-assist brake / damp velocity |
| M | switch Voyager mode |
| C | toggle fixed Voyager chase / FreeFly camera |
| T | trajectory toggle |
| O | orbit-line toggle |
| P | pause historical simulation |
| Esc | release mouse / quit depending on state |

We can change bindings after testing.

---

## Local axes

From quaternion orientation:

```cpp
glm::dvec3 forward =
    orientation * glm::dvec3(0.0, 0.0, -1.0);

glm::dvec3 right =
    orientation * glm::dvec3(1.0, 0.0, 0.0);

glm::dvec3 up =
    orientation * glm::dvec3(0.0, 1.0, 0.0);
```

Then:

```cpp
velocity += forward * thrust * dt;
position += velocity * dt;
```

Units must be defined consistently.

---

## Manual mode is not a full spacecraft physics simulator

Unless required, we do not need:

- propellant simulation,
- thruster nozzle allocation,
- rigid-body inertia tensor,
- real Voyager propulsion constraints.

The objective is **intuitive 3D navigation**, not a NASA flight dynamics simulator.

We can still make motion inertia-based and visually convincing.

---

# 35. Mode Switching

```cpp
enum class VoyagerMode
{
    Historical,
    Manual
};
```

## Historical → Manual

At switch instant:

1. retain current position,
2. obtain trajectory velocity,
3. retain/derive orientation,
4. initialize manual flight state from those values,
5. continue seamlessly.

This prevents teleportation.

---

## Manual → Historical

For the next lab:

### Simple behavior

```text
snap/reset to historical state for current simulation date
```

Later:

### Advanced behavior

```text
smooth rejoin transition
```

UI can explicitly label:

```text
Return to Historical Trajectory
```

so reset behavior is understandable.

---

# 36. Input System

Do not query GLFW keys separately in every class.

Centralize input.

```cpp
class Input
{
public:
    bool isKeyDown(int key) const;
    bool wasKeyPressed(int key) const;
    bool wasKeyReleased(int key) const;

    glm::dvec2 mouseDelta() const;
};
```

## Continuous vs edge-triggered input

### Continuous

```text
W held → thrust continuously
```

### Pressed once

```text
M pressed → toggle mode once
```

Without this distinction, holding `M` could switch modes every frame.

---

# 37. Camera System

At least four useful modes.

```cpp
enum class CameraMode
{
    Free,
    VoyagerChase,
    VoyagerForward,
    TargetFocus
};
```

---

## Free camera

Use standard FPS/orbital movement for exploring the scene.

---

## Voyager chase

Position camera behind Voyager.

```text
camera position =
voyager position
- forward * chaseDistance
+ up * chaseHeight
```

Smooth with interpolation.

---

## Voyager forward

Camera near spacecraft looking in the spacecraft's forward direction.

This creates a cockpit-like exploration view without needing an actual cockpit.

---

## Target focus

Select:

- Sun
- planet
- moon
- Voyager

and orbit/look around it.

---

## Near/far clipping

Huge scale causes depth-buffer issues.

Possible strategies:

1. dynamic near/far planes,
2. logarithmic depth later if necessary,
3. scale compression,
4. separate rendering passes for local and astronomical scales if absolutely needed.

Do not prematurely add a complex logarithmic-depth implementation.

First test the scale manager.

---

# 38. Trajectory Rendering

Render Voyager historical path as a line.

## Data source

Use the same sampled trajectory used by the historical controller.

```text
trajectory samples
      │
      ├── controller interpolation
      └── line-mesh generation
```

Avoid maintaining separate inconsistent trajectory data for motion and rendering.

## Useful rendering modes

```text
past path only
complete mission path
future path only
```

For the first lab:

```text
complete path toggle
```

is enough.

---

# 39. HUD, Labels, and Telemetry

HUD is not the main next-lab priority, but a minimal overlay makes the project easier to evaluate.

## Historical HUD

```text
VOYAGER 2 EXPLORER

Mode: HISTORICAL
Simulation Date: ...
Time Scale: ...x
Nearest / Selected Body: ...
Distance from Sun: ... AU
Trajectory: ON
Camera: CHASE
```

## Manual HUD

```text
Mode: MANUAL
Speed: ...
Position: ...
Distance from Sun: ...
Target: ...
Camera: ...
```

## Text implementation

Reuse any starter text renderer if supplied.

Otherwise choose a lightweight text system later.

Do not let UI implementation block object creation.

---

# 40. Object Selection and Focus

Recommended eventual interaction:

```text
number keys
or
simple selection menu
or
mouse picking
```

Next lab can use keyboard selection.

Example:

```text
1 Sun
2 Mercury
3 Venus
4 Earth
5 Mars
6 Jupiter
7 Saturn
8 Uranus
9 Neptune
V Voyager
```

Then:

```text
F1 / Enter → focus selected object
```

Avoid mouse ray-picking until core functionality is stable unless required.

---

# 41. Performance Strategy

## Shared meshes

One sphere mesh reused by many bodies.

## Shared textures/material resources

Asset manager caches resources by path/ID.

## Instancing

Use for:

- asteroid belt
- Kuiper belt
- repeated stars/particles if needed

## Level of detail

Possible later:

- high-detail Voyager near camera
- lower-detail version at distance

Not necessary for first implementation if asset complexity is manageable.

## Avoid per-frame allocation

Do not recreate vectors, meshes, shaders, or textures every frame.

Create GPU resources once.

---

# 42. Error Handling and Debugging

## GLFW error callback

Register one early.

```cpp
void glfwErrorCallback(int error, const char* description)
{
    std::cerr
        << "[GLFW] " << error
        << ": " << description
        << '\n';
}
```

## Shader errors

Every compile/link failure must print:

```text
shader path
stage
OpenGL info log
```

Never silently return a broken shader.

## Asset errors

Missing texture/model should report full resolved path.

Fallback:

```text
magenta/basic placeholder material
```

is better than a crash during the lab.

---

## OpenGL debug context

If the environment supports it, enable OpenGL debug output during development.

Useful for detecting:

- invalid enums
- invalid operations
- deprecated usage
- framebuffer problems

This can be added after the starter project baseline is stable.

---

# 43. Testing Strategy

This project needs visual tests and logical tests.

## T1 — startup

- window opens
- GLAD loads
- correct OpenGL version prints
- no shader compile failures

## T2 — base transform

- cube translates
- rotates
- scales
- parent/child transform works

## T3 — sphere

- normals correct
- no missing seam
- texture UVs correct

## T4 — multiple bodies

- Sun, Earth, Jupiter render simultaneously
- independent transforms

## T5 — hierarchy

- Moon follows Earth
- Saturn rings follow Saturn

## T6 — simulation clock

- pause works
- speed changes work
- same time is supplied to every historical component

## T7 — Voyager historical mode

- sample trajectory loads
- Voyager moves as simulation time changes
- trajectory line agrees with spacecraft path

## T8 — Voyager manual mode

- all six orientation/movement controls behave consistently
- rotation does not gimbal lock
- mode transition does not crash

## T9 — camera

- free camera
- chase camera
- forward camera
- target focus

## T10 — large coordinates

- no obvious shaking at large distances
- camera-relative transform remains stable

## T11 — belt performance

- target frame rate remains acceptable with representative instance count

---

# 44. Data and Configuration Files

## `celestial_bodies.json`

Recommended shape:

```json
{
  "bodies": [
    {
      "id": "earth",
      "name": "Earth",
      "type": "planet",
      "parent": "sun",
      "radiusKm": 6371.0,
      "semiMajorAxisKm": 149597870.7,
      "rotationPeriodHours": 23.934,
      "orbitalPeriodDays": 365.256,
      "axialTiltDegrees": 23.44,
      "texture": "textures/planets/earth.jpg"
    }
  ]
}
```

Exact values should later be verified before final presentation.

---

## `voyager2_trajectory.csv`

```csv
julian_date,x_km,y_km,z_km,vx_km_s,vy_km_s,vz_km_s
...
```

## `mission_bookmarks.json`

Potential:

```json
{
  "bookmarks": [
    {
      "name": "Launch",
      "date": "1977-08-20"
    },
    {
      "name": "Jupiter Flyby",
      "date": "1979-07-09"
    },
    {
      "name": "Saturn Flyby",
      "date": "1981-08-25"
    },
    {
      "name": "Uranus Flyby",
      "date": "1986-01-24"
    },
    {
      "name": "Neptune Flyby",
      "date": "1989-08-25"
    }
  ]
}
```

These mission dates are useful for demo navigation.

---

# 45. Implementation Phases

# Phase 0 — Preserve and audit starter project

**Goal:** Understand exactly what the instructor gave us.

Checklist:

- [x] Project compiles in Visual Studio.
- [x] Current cube appears.
- [x] Determine requested OpenGL version.
- [x] Identify GLAD generation/API.
- [x] Identify GLFW version/layout.
- [x] Identify shader files.
- [x] Identify texture loader.
- [x] Identify model loader.
- [x] Identify camera code.
- [x] Identify matrix library.
- [x] Inventory every supplied asset.
- [x] Create baseline Git commit/tag.

**Do not refactor before this succeeds.**

---

# Phase 1 — Core reusable scene foundation

**Goal:** Existing cube becomes one object in a reusable scene.

Implement:

- [x] `Application`
- [x] `Transform`
- [x] `SceneObject`
- [x] `Renderer` basic wrapper
- [x] basic `Camera`
- [x] central `Input`
- [x] delta time
- [x] cube moved out of `main.cpp`

Success test:

```text
Two cubes render with different transforms.
```

---

# Phase 2 — Sphere and material-ready mesh

Implement:

- [x] `Vertex` with position, normal, UV
- [x] reusable `Mesh`
- [x] UV sphere generation or clean imported sphere
- [x] texture-ready material
- [x] depth testing confirmed

Success:

```text
Sun + Earth + Jupiter render simultaneously.
```

---

# Phase 3 — Solar-system object architecture

Implement:

- [x] `CelestialBodyData`
- [x] `CelestialBody`
- [x] `SolarSystem`
- [x] body registry by ID
- [x] parent-child relationships
- [x] all eight planets created
- [x] minimum required moons

Success:

```text
Every required body can be toggled/focused and has an independent transform.
```

---

# Phase 4 — Scale Manager

Implement before full orbit distances.

- [x] physical radius
- [x] display radius
- [x] physical position
- [x] render position
- [x] educational distance mapping
- [x] spacecraft visibility scaling (`ScaleManager::spacecraftSizeToRenderUnits`, one linear mapping for all major sourced Voyager dimensions)

Success:

```text
Sun and all planets can be viewed in one useful scene without destroying physical values.
```

---

# Phase 5 — Orbit and rotation

- [x] planetary spin
- [x] circular-orbit temporary model
- [x] moon local orbits
- [x] orbit line mesh
- [x] simulation clock (`CelestialBody::setSimulationTimeScale`, shared static; `P` pause, `=`/`-` speed, see docs/objects/controls.md)

Success:

```text
Pause and speed controls affect the full system consistently.
```

---

# Phase 6 — Rings

- [x] annulus mesh (`RingGenerator`, double-sided)
- [x] Saturn rings (5 real named bands, real gap at the Cassini Division)
- [x] generic ring support (`buildRingSystem` takes any planet id + band list)
- [x] Uranus ring placeholder (built as 5 real bands, not just a placeholder)
- [x] Jupiter/Neptune support as required (3 faint bands each)

Success:

```text
rings inherit planet movement and orientation.
```

---

# Phase 7 — Asteroid and Kuiper belts

- [x] asteroid base mesh(es) (low-poly `UvSphereGenerator(6, 8)`, shared)
- [x] deterministic transform generation (seeded `std::mt19937` per field)
- [x] instanced rendering (`glDrawElementsInstanced` + `glVertexAttribDivisor`, see docs/objects/instancing.md)
- [x] asteroid belt (2,500 instances)
- [x] Kuiper belt (1,500 instances; Oort cloud also done, 800 instances, beyond this checklist's scope)

Success:

```text
large object counts render at acceptable frame rate.
```

---

# Phase 8 — Voyager model

- [x] author model procedurally (no imported mesh; `VoyagerModelBuilder` composes project-native generators from NASA references)
- [x] establish local forward/up axis (`Voyager2`'s local +Z = heading)
- [x] root transform
- [x] material assignment (flat colors per part)
- [x] major component hierarchy (parabolic HGA/feed/LGA, decagonal bus, three lattice booms, finned RTGs, scan platform/instruments, magnetometers, PRA/PWS antennas, panels, record, 16 thrusters)
- [x] model display scaling (`ScaleManager::spacecraftSizeToRenderUnits` for every major sourced dimension)

Success:

```text
Voyager can be placed near Earth and inspected from multiple camera angles.
```

---

# Phase 9 — Manual flight first

Manual control is easier to validate than historical ephemeris and directly satisfies an instructor requirement.

- [x] `VoyagerController` (control logic lives directly in `Voyager2::applyManualControl` instead of a separate controller class — functionally equivalent, different class split)
- [x] `ManualFlightController` (same as above)
- [x] quaternion orientation (full 6-DOF `m_orientation * turn`, no Euler state)
- [x] forward thrust (`W`/`S`, along current facing, inertial)
- [x] yaw (`A`/`D` or arrow keys)
- [x] pitch (`R`/`F`, about the ship's own +X axis)
- [x] roll (`Q`/`E`, about the ship's own +Z axis)
- [x] optional strafing (`Space`/`Left Ctrl` vertical thrust)
- [x] velocity damping (explicit `X` braking burn; passive drag stays absent because space has none)
- [x] chase camera (ThirdPerson mode)

Success:

```text
Student can pilot Voyager intuitively through 3D space.
```

---

# Phase 10 — Historical trajectory foundation

- [x] `TrajectorySample`
- [x] `Trajectory`
- [x] offline NASA/JPL Horizons sample file
- [x] interpolation
- [x] historical controller
- [x] simulation-clock coupling (pause/speed drive historical playback; manual input remains real-time)
- [x] line rendering
- [x] mode toggle

Success:

```text
Voyager automatically follows a visible trajectory and manual/historical switching works.
```

---

# Phase 11 — Official trajectory / synchronized ephemeris

- [x] obtain NASA/JPL SPICE-derived data (Horizons API serves the SPICE solutions; kernels themselves not needed offline)
- [x] build offline preprocessing utility/script (`scripts/fetch_horizons.ps1`)
- [x] export sampled Horizons trajectory CSV (11,002 state vectors, 1-minute at each closest approach)
- [x] verify coordinate frame (ECLIPTIC X/Y -> scene X/Z, ECLIPTIC Z -> scene Y)
- [x] sample celestial ephemeris or derive synchronized body positions (all 9 planets, Hermite-interpolated on one `SimulationClock`)
- [x] verify flyby proximity (`verify_scene_layout.ps1`: within 0.5 % and 1 minute of published closest approaches)

Success:

```text
Voyager meets each giant planet near the correct historical mission date.
```

---

# Phase 12 — Outer boundary objects

- [x] heliosphere / termination-shock wireframe placeholder
- [x] heliopause wireframe marker
- [x] interstellar-space representation (heliopause exterior + starfield)
- [x] labels (depth-sorted, occlusion-tested body names; see docs/objects/hud-overlay.md)

---

# Phase 13 — UI / presentation controls

- [x] HUD (project-authored 5x7 font, screen-space text shader; date, rate, telemetry, encounter banner, F1 help)
- [x] selected object
- [x] mode display
- [x] historical Julian Date
- [x] time scale
- [x] trajectory toggle (`T`)
- [x] camera mode display
- [x] mission bookmarks (1-6: Launch, four giant-planet encounters, Interstellar)

---

# Phase 14 — Instructor lighting/shader integration

Only after object architecture is stable.

- [x] inspect supplied shader interfaces (none supplied; project uniforms documented in docs/objects/lighting.md so instructor shaders can map onto them)
- [x] map shader uniforms to `Material` (`shading`, `specularStrength`, `specularPower`, `opacity`)
- [x] Sun emissive behavior (unlit Sun + two additive glow shells)
- [x] planet lighting (Sun point light, Lambert + Blinn-Phong; `K` toggles)
- [x] shading techniques: Flat, Gouraud, Phong, Blinn-Phong, Toon (`F3`); spot headlamp (`F5`), directional fill (`F6`), Sun falloff (`F7`)
- [x] normal/specular support (maps derived from the albedo photographs, `F8`; tangents from screen-space derivatives)
- [x] ring blending (translucent two-sided rings)
- [x] space background (three camera-centred star layers)
- [x] shadows: ray-traced hard and soft Sun shadows from spheres, rings and Voyager's BVH (`F4`); full Whitted ray-traced view (`F9`, docs/objects/ray-tracing.md)
- [x] final visual polishing (floating origin, logarithmic depth, larger display radii)

---

# 46. Next-Lab 50% Evaluation Definition of Done

Use this section as the actual lab gate.

## Critical — must be ready

- [x] Starter project still compiles reliably in Visual Studio.
- [x] GLAD and GLFW initialization are clean.
- [x] Rendering is no longer cube-only architecture.
- [x] Reusable mesh system exists.
- [x] Reusable transform exists.
- [x] Scene hierarchy exists.
- [x] Sphere mesh exists.
- [x] Sun exists.
- [x] Mercury exists.
- [x] Venus exists.
- [x] Earth exists.
- [x] Mars exists.
- [x] Jupiter exists.
- [x] Saturn exists.
- [x] Uranus exists.
- [x] Neptune exists.
- [x] Important moons are represented (12 required + 4 optional Saturn moons, all 21+4).
- [x] Saturn's ring system exists.
- [x] Generic ring architecture exists (also used for Uranus/Jupiter/Neptune).
- [x] Asteroid belt exists.
- [x] Kuiper-belt representation exists.
- [x] Star/background object exists (4,000-point starfield).
- [x] Voyager 2 model exists.
- [x] Voyager is transformable as one root object.
- [x] Manual Voyager control foundation works.
- [x] Historical trajectory controller foundation exists (160 offline NASA/JPL Horizons heliocentric samples with interpolation).
- [x] Voyager mode switch exists (`V`: Historical/Manual).
- [x] A true Voyager chase camera starts directly behind the flight direction; FreeFly is one-key accessible.
- [x] Scale system allows both planets and Voyager to be demonstrated.
- [x] No major object category from the planned pre-lighting scene is completely absent.

## Strongly recommended

- [x] trajectory line (same Horizons samples as Historical playback)
- [x] planetary orbit lines
- [x] simulation clock (pause/speed controller; date-based ephemeris remains Phase 10–11)
- [x] mission bookmarks (1-6)
- [x] labels
- [x] heliosphere/termination-shock placeholder (three-axis wire boundary)
- [x] heliopause placeholder (three-axis wire boundary)
- [x] multiple cameras (ThirdPerson, Focus, FreeFly)

## Can wait for later shader lab

- [ ] bloom
- [ ] advanced Sun effect
- [ ] atmospheric scattering
- [ ] detailed shadow system
- [ ] physically based materials
- [ ] advanced ring transparency
- [ ] post-processing

---

# 47. Later Lighting and Shader Integration

The instructor is expected to provide lighting/shader resources later.

## Integration philosophy

Wrong:

```text
copy lighting code into Earth
copy lighting code into Mars
copy lighting code into Jupiter
...
```

Correct:

```text
Shader
  │
Material
  │
Renderer
  │
All renderable objects
```

## Uniform preparation

Likely categories:

```text
model matrix
view matrix
projection matrix
normal matrix
camera position
light position
light color
material properties
textures
```

The exact names must match the instructor's files.

Do not prematurely invent incompatible uniform names across the entire project.

---

# 48. Presentation / Demonstration Flow

A strong final demo sequence:

```text
1. Launch application
        │
        ▼
2. Wide solar-system view
        │
        ▼
3. Focus Sun / planets
        │
        ▼
4. Show orbit guides
        │
        ▼
5. Select Voyager 2
        │
        ▼
6. HISTORICAL MODE
        │
        ├─ Launch
        ├─ Jupiter
        ├─ Saturn
        ├─ Uranus
        ├─ Neptune
        └─ outbound / interstellar
        │
        ▼
7. Show full mission trajectory
        │
        ▼
8. Switch HISTORICAL → MANUAL
        │
        ▼
9. Chase camera
        │
        ▼
10. Pilot Voyager
        │
        ├─ thrust
        ├─ pitch
        ├─ yaw
        └─ roll
        │
        ▼
11. Fly toward/focus a selected body
        │
        ▼
12. End in outer solar system / interstellar view
```

This clearly demonstrates the project's two main experiences.

---

# 49. Coding Standards

## Naming

Suggested:

```cpp
class SolarSystem;
class ManualFlightController;

void updateSimulation();
double simulationTime;

m_memberVariable;
```

Stay consistent with the instructor project if it already has a style.

---

## Headers

Prefer:

```cpp
#pragma once
```

if already used.

Minimize heavy includes.

Use forward declarations where practical.

---

## Constants

Avoid magic numbers.

Bad:

```cpp
position *= 0.0000042;
```

Better:

```cpp
position *= scaleManager.distanceScale();
```

---

## Ownership

Prefer clear RAII.

Use:

```cpp
std::unique_ptr
std::shared_ptr
std::vector
```

where appropriate.

Avoid raw `new` / `delete` scattered through the application.

OpenGL resource classes should clean themselves up.

Example:

```cpp
Mesh::~Mesh()
{
    glDeleteVertexArrays(...);
    glDeleteBuffers(...);
}
```

Be careful when objects are copied; GPU resource classes may need move-only semantics.

---

## Const correctness

Functions that only inspect state should be `const`.

---

## Logging

Use categories:

```text
[APP]
[GLFW]
[OPENGL]
[SHADER]
[ASSET]
[SCENE]
[SOLAR]
[VOYAGER]
[TRAJECTORY]
```

This will help greatly during lab debugging.

---

# 50. Git and Change Discipline

Recommended branches:

```text
main
develop
feature/scene-foundation
feature/solar-system
feature/voyager-manual
feature/voyager-trajectory
feature/camera
feature/belts
feature/shaders
```

For a small student team, fewer branches are fine, but commits should still be focused.

## Useful checkpoints

```text
starter-working
scene-objects-working
solar-system-basic
voyager-manual-working
voyager-historical-working
pre-shader-lab
final-demo
```

## Do not commit

- Visual Studio temporary caches
- large build folders
- generated debug binaries unless required
- duplicate texture/model copies

Create an appropriate `.gitignore`.

---

# 51. Common Failure Modes to Avoid

## F1 — Everything in `main.cpp`

This blocks scaling and debugging.

---

## F2 — One class per planet with copied renderer

Unnecessary duplication.

---

## F3 — Using only floats for astronomical state

Creates precision trouble.

---

## F4 — Using literal physical scale directly for rendering

Planets disappear or distances become unusable.

---

## F5 — Historical Voyager with fake/current planet positions

Flybys will not line up.

---

## F6 — Manual mode changes Euler angles directly forever

Can produce rotation/gimbal problems.

Use quaternion orientation.

---

## F7 — Input checks spread through every object

Creates unmaintainable control code.

---

## F8 — One OpenGL VAO/VBO created every frame

Major performance/resource bug.

---

## F9 — Thousands of asteroid `SceneObject` draw calls

Use instancing.

---

## F10 — Lighting implemented before geometry architecture

Then shaders become entangled with unfinished objects.

---

## F11 — Hard-coded absolute Windows paths

Breaks on instructor/lab computers.

---

## F12 — Starter project rewritten from scratch unnecessarily

Risky and wastes instructor-provided foundation.

---

## F13 — Two separate Voyager models for the two modes

Violates clean controller architecture and complicates switching.

---

## F14 — Simulation-speed multiplier applied accidentally to camera/input

At `10000x`, controls become unusable.

Separate real delta from simulation time.

---

# 52. Recommended Class Interfaces

These are conceptual starting points, not final APIs.

## Application

```cpp
class Application
{
public:
    bool initialize();
    int run();
    void shutdown();

private:
    void update(double dt);
    void render();

    GLFWwindow* m_window = nullptr;

    Input m_input;
    SimulationClock m_clock;
    Scene m_scene;
    Renderer m_renderer;
    CameraSystem m_cameraSystem;
};
```

---

## Transform

```cpp
struct Transform
{
    glm::dvec3 position {0.0};
    glm::dquat rotation {1.0, 0.0, 0.0, 0.0};
    glm::dvec3 scale {1.0};

    glm::dmat4 matrix() const;
};
```

---

## SceneObject

```cpp
class SceneObject
{
public:
    virtual ~SceneObject() = default;

    virtual void update(double dt);
    virtual void render(Renderer& renderer);

    Transform localTransform;

    SceneObject* parent = nullptr;
    std::vector<std::unique_ptr<SceneObject>> children;
};
```

Actual ownership may use IDs/shared resources.

---

## Mesh

```cpp
class Mesh
{
public:
    Mesh(
        std::vector<Vertex> vertices,
        std::vector<unsigned int> indices
    );

    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&&) noexcept;
    Mesh& operator=(Mesh&&) noexcept;

    void draw() const;

private:
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_ebo = 0;
    int m_indexCount = 0;
};
```

---

## Camera

```cpp
class Camera
{
public:
    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspect) const;

    glm::dvec3 worldPosition;
    glm::dquat orientation;

    float fovDegrees = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 10000.0f;
};
```

Near/far values become more sophisticated with scale modes.

---

## ScaleManager

```cpp
enum class ScaleMode
{
    Educational,
    ApproximatePhysical,
    LocalEncounter
};

class ScaleManager
{
public:
    glm::dvec3 mapRelativePositionKm(
        const glm::dvec3& relativeKm
    ) const;

    double mapBodyRadiusKm(double radiusKm) const;

    double mapSpacecraftMeters(double meters) const;

    void setMode(ScaleMode mode);
};
```

---

## SimulationClock

```cpp
class SimulationClock
{
public:
    void update(double realDeltaSeconds);

    void setPaused(bool paused);
    void setTimeScale(double daysPerRealSecond);

    double julianDate() const;
    void setJulianDate(double value);

private:
    double m_julianDate = 0.0;
    double m_daysPerRealSecond = 1.0;
    bool m_paused = false;
};
```

---

## CelestialBody

```cpp
class CelestialBody : public SceneObject
{
public:
    CelestialBody(
        CelestialBodyData data,
        std::shared_ptr<Mesh> sphere,
        std::shared_ptr<Material> material
    );

    void update(double dt) override;
    void render(Renderer& renderer) override;

private:
    CelestialBodyData m_data;
    std::shared_ptr<Mesh> m_mesh;
    std::shared_ptr<Material> m_material;
};
```

---

## Voyager 2

```cpp
class Voyager2 : public SceneObject
{
public:
    void update(double dt) override;

    void setMode(VoyagerMode mode);

    void setPhysicalPositionKm(const glm::dvec3& position);
    const glm::dvec3& physicalPositionKm() const;

    void setVelocityKmPerSec(const glm::dvec3& velocity);
    const glm::dvec3& velocityKmPerSec() const;

    void setOrientation(const glm::dquat& q);
    const glm::dquat& orientation() const;

private:
    VoyagerMode m_mode = VoyagerMode::Historical;

    glm::dvec3 m_positionKm {0.0};
    glm::dvec3 m_velocityKmPerSec {0.0};
    glm::dquat m_orientation {1.0, 0.0, 0.0, 0.0};

    std::unique_ptr<VoyagerController> m_activeController;
};
```

---

## Manual Controller

```cpp
class ManualFlightController : public VoyagerController
{
public:
    ManualFlightController(const Input& input);

    void onActivated(Voyager2& voyager) override;

    void update(
        Voyager2& voyager,
        double dt
    ) override;

private:
    const Input& m_input;

    double m_thrustAcceleration = 1.0;
    double m_rotationRate = 1.0;
};
```

---

## Historical Controller

```cpp
class RealTrajectoryController : public VoyagerController
{
public:
    RealTrajectoryController(
        const SimulationClock& clock,
        const Trajectory& trajectory
    );

    void update(
        Voyager2& voyager,
        double dt
    ) override;

private:
    const SimulationClock& m_clock;
    const Trajectory& m_trajectory;
};
```

---

# 53. First Concrete Implementation Sprint

This is where implementation should begin once the real starter-project files are available.

## Sprint objective

Go from:

```text
starter cube
```

to:

```text
clean reusable scene foundation + three celestial objects
```

## Exact steps

### Step 1
Open the Visual Studio solution.

Record:

```text
OpenGL version:
GLAD style/version:
GLFW layout:
GLM present?:
Shader files:
Texture loader:
Model loader:
Main source files:
Current controls:
```

### Step 2
Build original project unchanged.

### Step 3
Create baseline Git commit.

### Step 4
Extract window initialization into a clean application boundary only if the starter code needs it.

### Step 5
Create/identify `Shader`.

Do not rewrite working shader code unnecessarily.

### Step 6
Create `Transform`.

### Step 7
Move cube geometry into `Mesh` or adapt the instructor's existing mesh abstraction.

### Step 8
Render two cubes independently.

This proves reusable transforms.

### Step 9
Create UV sphere.

### Step 10
Render a sphere using temporary color.

### Step 11
Add normal and UV vertex attributes even if the current shader does not yet consume all of them.

### Step 12
Create:

```text
Sun
Earth
Jupiter
```

using the same sphere mesh and different transforms/material data.

### Step 13
Add basic camera movement.

### Step 14
Commit:

```text
scene-foundation-working
```

## Sprint completion rule

Do not add all planets before these three objects prove that the architecture is genuinely reusable.

---

# 54. Backlog

## Core

- [x] Application wrapper
- [x] Window wrapper
- [x] Input system
- [x] logging
- [ ] asset manager
- [x] transform
- [x] scene graph

## Renderer

- [x] shader abstraction
- [x] mesh
- [x] texture
- [x] material
- [ ] model
- [x] line renderer
- [x] instanced renderer

## Solar system

- [x] Sun
- [x] Mercury
- [x] Venus
- [x] Earth
- [x] Mars
- [x] Jupiter
- [x] Saturn
- [x] Uranus
- [x] Neptune
- [x] Pluto optional
- [x] Moon
- [x] Galilean moons
- [x] Titan
- [x] Uranian major moons
- [x] Triton
- [x] rings
- [x] asteroid belt
- [x] Kuiper belt
- [x] orbit lines

## Voyager

- [x] procedural model construction (used instead of importing a finished model)
- [x] hierarchy
- [x] forward/up convention
- [x] manual controller
- [x] historical controller
- [x] trajectory line
- [x] mode switch
- [x] chase camera
- [ ] forward camera
- [x] teleport/reset/rejoin behavior (bookmarks teleport; `V` back to Historical rejoins the dated path)

## Time / astronomy

- [x] simulation clock
- [x] mission bookmarks
- [x] planet position model
- [x] SPICE preprocessing (via Horizons API, `scripts/fetch_horizons.ps1`)
- [x] trajectory CSV
- [x] synchronized historical ephemeris

## Large world

- [x] physical-vs-render positions
- [x] scale manager
- [x] educational scale
- [x] local encounter scale (flyby clearance, docs/objects/mission-ephemeris.md)
- [x] floating origin

## UI

- [x] mode
- [x] date
- [x] selected object
- [x] distance from Sun
- [x] velocity
- [x] simulation scale
- [x] control help

## Outer region

- [x] heliosphere
- [x] termination shock
- [x] heliopause
- [x] interstellar background

## Final graphics

- [ ] instructor shaders
- [x] lighting
- [x] Sun material
- [x] ring transparency
- [x] planet material polish (per-material specular)
- [ ] post-processing if allowed
- [x] final screenshots/demo

---

# 55. Decision Log

Use this table whenever we make a significant architecture change.

| ID | Date | Decision | Reason | Impact |
|---|---|---|---|---|
| D001 | 2026-09-09 | One Voyager object with two controllers | Clean mode switching; meets instructor requirement | Voyager architecture |
| D002 | 2026-09-09 | Separate physical and render scale | Solar-system distances cannot use one practical visual scale | All spatial systems |
| D003 | 2026-09-09 | Use double precision for simulation position | Large-world stability | Transform/coordinate design |
| D004 | 2026-09-09 | Historical Voyager and planets share SimulationClock | Required for actual flyby alignment | Ephemeris/trajectory |
| D005 | 2026-09-09 | Keep objects shader-ready but defer advanced shader work | Next lab prioritizes objects; instructor shaders arrive later | Renderer/material design |
| D006 | 2026-09-09 | Preserve starter project's requested OpenGL context version | Avoid compatibility break with instructor code/lab PCs | Initialization |
| D007 | 2026-09-13 | Generate one 32×64 indexed UV sphere and share it across bodies | Keeps geometry reusable while providing normals, UVs, a clean seam, and non-degenerate poles | Mesh/rendering |
| D008 | 2026-09-13 | Vendor fixed NASA Earth, Jupiter, and Sun maps with a provenance manifest | Runtime must be deterministic and offline while scientific context and credits remain auditable | Assets/materials |
| D009 | 2026-09-13 | Require a separate illustrated Markdown guide for every renderable object | Makes geometry, transforms, textures, and limitations explainable during assessment | Documentation/all object phases |
| D010 | 2026-09-13 | Replace D008's downloaded runtime maps with original procedural RGBA recipes | Course work requires self-made objects/assets; external imagery is reference-only unless the instructor explicitly permits reuse | Assets/materials |
| D011 | 2026-09-21 | Reverse D010 for surface textures only: use credited real imagery while keeping every mesh self-authored | User explicitly permits textures/assets as reference and requires self-made objects, not self-photographed planet surfaces | Assets/materials; geometry remains procedural |
| D012 | 2026-09-21 | Rebuild Voyager through `VoyagerModelBuilder` instead of importing NASA's downloadable mesh | Meets the course authorship rule while restoring real proportions and recognizable hardware | Voyager geometry/documentation |
| D013 | 2026-09-21 | Vendor sparse NASA/JPL Horizons vectors and interpolate them offline | Historical path is auditable and works on lab machines without network access | Trajectory/data/runtime |
| D014 | 2026-09-24 | Replace sparse tracks with dense Horizons state vectors for all planets and Voyager, cubic-Hermite interpolated on one `SimulationClock` | Flybys must happen at the real time and side; planets must not be animated by a private clock | MissionEphemeris, SimulationClock |
| D015 | 2026-09-24 | Compose encounter-window Voyager rows as planet + planet-centred vectors | Horizons' heliocentric Voyager and planet solutions disagree by ~13,600 km at Neptune | Trajectory data |
| D016 | 2026-09-24 | Power-law body radii (Earth 0.5, exponent 0.6) and a display-capped Sun instead of one linear radius factor | The linear scale made planets sub-pixel and the scene look empty; size order is kept | ScaleManager/all bodies |
| D017 | 2026-09-24 | Floating origin plus logarithmic depth | Lets one frame hold 2 cm spacecraft struts and the Oort cloud without per-mode clip planes | Renderer/shaders |
| D018 | 2026-09-24 | Implement Sun lighting on the existing Material seam (supersedes D005's deferral) | No instructor shaders were supplied; readable day/night sides were needed | Material, shaders |
| D019 | 2026-09-25 | Five selectable shading techniques and three light types in one shared `lighting.glsl` | The course's shading topics must be demonstrable side by side on the same objects without per-object shaders | shaders, LightingController |
| D020 | 2026-09-25 | Derive normal and specular maps from the albedo photographs; rebuild tangents from screen-space derivatives | No bump maps were supplied, and the shared `Vertex` format must not grow a tangent attribute | SurfaceMaps, scene.frag |
| D021 | 2026-09-25 | Analytic ray tracing (spheres, annuli, circle-overlap soft shadows) plus a Whitted ray-traced view that writes log depth | Real ray tracing for extra credit that composites with the raster scene and stays interactive | raytrace.glsl, RayTracer |
| D022 | 2026-09-25 | Ray-trace Voyager's own triangles through a median-split BVH in texture buffers | Voyager must receive self-shadows and appear in the traced view; GLSL 3.30 has no SSBOs | TriangleBvh |
| D023 | 2026-09-25 | Texture Voyager from NASA's public-domain atlas via per-material UV windows; geometry stays self-authored | Real hardware finishes without importing another project's geometry | MaterialLibrary, VoyagerModelBuilder |
| D024 | 2026-09-25 | Spacecraft scale 0.006 units/m, 18 inspectable components and an Inspect camera | Voyager is the primary object and must be studied part by part | ScaleManager, CameraController |
| D025 | 2026-09-25 | Data-driven catalogs (`assets/data/*.csv`) and named scene groups | Bodies, rings and bookmarks can be changed without recompiling; groups toggle whole categories | BodyCatalog, Scene |

Add entries below rather than deleting old decisions. If a decision is reversed, add a new decision referencing the old one.

---

# 56. Implementation Session Log Template

At the end of each development session, append a small record.

```markdown
## Session YYYY-MM-DD

### Goal
What we intended to implement.

### Files changed
- ...
- ...

### Completed
- [x] ...

### Problems found
- ...

### Decisions made
- ...

### Current build state
- Builds: Yes/No
- Runs: Yes/No
- Known errors: ...

### Next exact task
...
```

This will make it much easier to continue implementation without forgetting why code was structured a certain way.

## Session 2026-09-24

### Goal
Finish the remaining phases: synchronized ephemeris and verified flybys (11), labels (12), HUD (13) and lighting (14); give the teacher full camera autonomy; make the bodies larger on screen.

### Files changed
- New: `src/scene/MissionEphemeris.*`, `src/scene/SimulationClock.*`, `src/rendering/BitmapFont.*`, `src/rendering/TextRenderer.*`, `hud.vert`, `hud.frag`, `scripts/fetch_horizons.ps1`, `docs/objects/{mission-ephemeris,lighting,hud-overlay}.md`
- Rewritten: `Application`, `Camera`, `Voyager2`, `Renderer`, `ScaleManager`, `Trajectory`, `default.vert/.frag`, all trajectory CSVs, both verify scripts, every object doc

### Completed
- [x] Dense Horizons ephemeris for 9 planets + Voyager; closest approaches within 0.5 % / 1 minute of NASA's values
- [x] Encounter slow-motion, bookmarks at each closest approach, flyby-facing chase camera
- [x] Free flight with adaptive + wheel speed, Focus fly-to on all 26 bodies, orbit/zoom rigs, 6-DOF manual flight
- [x] Sun lighting, glow, translucent rings, floating origin, logarithmic depth
- [x] HUD, labels, help overlay, screenshot and scripted capture tours

### Problems found
- Heliocentric Voyager vs heliocentric Neptune from Horizons disagree by ~13,600 km (D015).
- Without vsync, frame-count capture waits finished before camera transitions.

### Decisions made
- D014-D018.

### Current build state
- Builds: Yes (Debug and Release x64, no warnings from new code)
- Runs: Yes; `verify_scene_layout.ps1` and `verify_navigation_and_motion.ps1` pass
- Known errors: none

### Next exact task
Optional polish: shadows, date-synchronized moons, instructor shader swap-in when supplied.

## Session 2026-09-25

### Goal
Complete the lighting and shading phase without instructor resources, add ray tracing, overhaul controls, organise the scene as data, make Voyager 2 detailed, textured and inspectable, and document everything for learning.

### Files changed
- New: `shaders/` (scene, lighting, raytrace, raytrace_mesh, hud), `src/core/{LightingController,CameraController,CaptureTour}.*`, `src/rendering/{ShaderProgram,LightingUniforms,SurfaceMaps,MaterialLibrary,RayTracer,TriangleBvh}.*`, `src/scene/{BodyCatalog,SolarSystemBuilder,EnvironmentBuilder,MissionController,Comet}.*`, `assets/data/*.csv`, `assets/textures/spacecraft/`, `docs/guide/`, `docs/objects/{ray-tracing,voyager-textures}.md`
- Rewritten: `Application`, `Renderer`, `VoyagerModelBuilder`, `Voyager2`, `HudOverlay`, both verify scripts, lighting/controls/voyager/HUD docs

### Completed
- [x] Flat, Gouraud, Phong, Blinn-Phong and Toon shading; point, spot and directional lights; attenuation; normal and specular maps
- [x] Ray-traced hard and soft Sun shadows; Whitted ray-traced view with ring transparency and reflections; Voyager BVH (11,652 triangles)
- [x] Click-to-select, planet/moon stepping, Inspect mode over 18 Voyager components, key-hint bar, safe quit
- [x] Voyager rebuilt (82 parts) and textured from NASA's atlas; self-shadowing
- [x] Learning guide (10 chapters) and refreshed object docs with new capture tours

### Problems found
- The dish intersected the bus and straight ribs cut through the bowl; exposed by the ray tracer and fixed.

### Decisions made
- D019-D025.

### Current build state
- Builds: Yes (Debug and Release x64)
- Runs: Yes; both verify scripts pass
- Known errors: none

### Next exact task
Optional: date-synchronized moons, shadows from the headlamp, path-traced global illumination.

---

# 57. Reference Sources

These should be treated as primary technical/scientific references when we implement the relevant pieces.

## GLFW

Official documentation:

- https://www.glfw.org/docs/latest/
- https://www.glfw.org/docs/latest/quick_guide.html
- https://www.glfw.org/docs/latest/context_guide.html

GLFW provides window creation, OpenGL context handling, input, events, timers, and related platform-independent functionality.

## GLAD

Project/generator repository:

- https://github.com/Dav1dde/glad

The exact initialization call depends on the generated GLAD files included in our starter project. We will inspect those files before changing loader code.

## GLM

Official repository:

- https://github.com/g-truc/glm

GLM provides OpenGL/GLSL-style vectors, matrices, transforms, and quaternion support and is appropriate for the transform/camera/manual-flight math in this project.

## NASA Voyager 2

NASA Science mission page:

- https://science.nasa.gov/mission/voyager/voyager-2/

Use NASA as the primary source for mission history and encounter dates.

Key historical encounter sequence used by this project:

```text
Launch        1977
Jupiter       1979
Saturn        1981
Uranus        1986
Neptune       1989
Interstellar  2018
```

## NASA/JPL NAIF SPICE

NAIF data portal:

- https://naif.jpl.nasa.gov/naif/data.html
- https://naif.jpl.nasa.gov/naif/data_outer.html

NAIF provides SPICE kernel data for mission/solar-system geometry, including Voyager-related kernel collections.

### Project policy for SPICE

Prefer:

```text
SPICE kernels
    ↓
offline preprocessing
    ↓
small deterministic CSV/data files
    ↓
OpenGL application
```

rather than making the real-time renderer depend directly on the complete SPICE toolkit unless there is a compelling reason.

This keeps the class project:

- easier to build,
- easier to submit,
- easier to run on lab computers,
- and easier to debug.

---

# Final Implementation Principle

Whenever we are uncertain where a new feature belongs, use this test:

```text
Is it about physical state?
    → simulation / scene

Is it about drawing?
    → rendering

Is it about user commands?
    → input / controller

Is it about where we look?
    → camera

Is it astronomical source data?
    → data / ephemeris

Is it only a visual exaggeration?
    → ScaleManager

Is it Voyager-specific behavior?
    → voyager/controller

Is it reusable by many bodies?
    → generic system, not a planet-specific class
```

The final project should therefore evolve in this order:

```text
WORKING STARTER PROJECT
        ↓
CLEAN REUSABLE FOUNDATION
        ↓
COMPLETE OBJECT SET
        ↓
SOLAR-SYSTEM MOTION + SCALE
        ↓
VOYAGER MANUAL CONTROL
        ↓
VOYAGER HISTORICAL TRAJECTORY
        ↓
SYNCHRONIZED EPHEMERIS
        ↓
OUTER-SOLAR-SYSTEM EXPERIENCE
        ↓
INSTRUCTOR LIGHTING + SHADERS
        ↓
POLISH + PRESENTATION
```

> **The next objective is not "make it beautiful."  
> The next objective is "make every required object exist inside the correct architecture."**

Once that is achieved, lighting and shaders become an enhancement layer rather than a rescue operation.
