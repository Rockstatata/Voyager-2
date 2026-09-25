# 3. Transforms, cameras and the scene graph

[Guide index](README.md) · previous: [Geometry](02-geometry.md) · next: [Textures](04-textures.md)

A generator produces a mesh in its own **model space**: a unit sphere around (0,0,0), a box around the origin. This chapter explains how that mesh reaches the right place on screen.

```text
model space --model matrix--> world space --view matrix--> eye space --projection--> clip space --(÷w)--> screen
```

## 3.1 The model matrix: translate × rotate × scale

Every `SceneObject` has a `Transform` (src/scene/Transform.h) in **double precision**:

```cpp
struct Transform
{
    glm::dvec3 position{ 0.0 };
    glm::dquat rotation{ 1.0, 0.0, 0.0, 0.0 };   // quaternion; identity = no rotation
    glm::dvec3 scale{ 1.0 };

    glm::dmat4 localMatrix() const
    {
        return translate(position) * mat4_cast(rotation) * scale(scale);   // T * R * S
    }
};
```

A column vector is multiplied from the right, so `T·R·S·v` applies **scale first, then rotation, then translation**. That order matters. Scaling after translating would scale the position too, and rotating after translating would swing the object around the world origin instead of spinning it in place.

**Quaternions** store rotations. `angleAxis(angle, axis)` makes one, `q1 * q2` combines two, `slerp` interpolates between two smoothly, and `mat4_cast` turns one into a matrix. Unlike yaw/pitch/roll angles they have no gimbal lock, which is why Voyager's 6-DOF manual flight uses them.

## 3.2 Normals need a different matrix

When a mesh is scaled unevenly (a box stretched into a thin panel), transforming its normals by the model matrix bends them away from perpendicular. The correct matrix for normals is the **inverse-transpose** of the upper 3×3:

```glsl
normal = mat3(transpose(inverse(effectiveModel))) * aNormal;   // shaders/scene.vert
```

For a pure rotation, the inverse-transpose *is* the rotation. For a uniform scale it only changes the length, and `normalize` fixes that. The same formula appears on the CPU wherever geometry is baked: `appendTransformed`, and `TriangleBvh::addMesh`.

## 3.3 The scene graph: parents and children

```text
Scene
 ├── group "bodies"       (SolarSystem root)
 │    ├── sun
 │    ├── earth ── moon
 │    ├── jupiter ── io, europa, ganymede, callisto, ring bands
 │    └── ...
 ├── group "spacecraft"   └── voyager2 ── 82 parts (bus, dish, booms...)
 ├── group "mission_path" (trajectory line)
 ├── group "orbit_guides", "heliosphere", "small_bodies", "comet"
```

- Parents **own** their children (`std::vector<std::unique_ptr<SceneObject>>`). A child's `m_parent` is a plain, non-owning pointer back up.
- `worldMatrix()` = `parent->worldMatrix() * localMatrix()`. A moon's transform is expressed **relative to its planet**, and the planet's matrix carries it along.
- `Scene::group(name)` creates or returns a named root. Toggling a whole category (O hides orbit guides) is one `setVisible` call.

### The spin trick (CelestialBody)

A planet's *transform* holds its position, its scale and only its **axial tilt**. The daily **spin** is applied to the planet's own mesh at draw time:

```cpp
glm::dmat4 CelestialBody::surfaceMatrix() const
{
    return worldMatrix() * mat4_cast(angleAxis(m_spinAngleRadians, dvec3(0, 1, 0)));
}
```

If the spin were in the transform, every moon would be dragged around the planet once per planetary day. With the spin kept separate, children inherit the tilted *equatorial frame*, so the moons orbit in the equator plane, but they do not inherit the rotation.

### Moons are scaled by their parent

A moon's local position and scale are expressed in units of the **parent's render radius**, because the parent's scale multiplies everything below it. `SolarSystemBuilder` divides by the parent radius for this reason. See [scale-manager.md](../objects/scale-manager.md).

## 3.4 The camera: the view matrix

`Camera` (src/rendering/Camera.cpp) stores a **position** (`dvec3`) and three unit axes: `forward`, `right` and `up`. In free flight they come from yaw and pitch:

```cpp
forward = (cos(pitch)·cos(yaw), sin(pitch), cos(pitch)·sin(yaw));
right   = normalize(cross(forward, worldUp));
up      = normalize(cross(right, forward));
```

The **view matrix** moves the world so the eye is at the origin looking down −Z: `glm::lookAt(eye, eye + forward, up)`. This project always uses the eye at **(0,0,0)** (`viewMatrixAtOrigin`), because of the floating origin below.

## 3.5 Floating origin: why double precision and "camera-relative"

A `float` has about 7 significant digits. Neptune is thousands of units from the Sun, but Voyager's parts are **0.0003 units** apart: a 5 cm antenna at 0.006 units per metre. At a coordinate of 3000, the smallest step a float can represent is about 0.0002, so Voyager would shake and its parts would snap together.

The fix, in `Renderer::submit`:

```cpp
glm::dmat4 relative = worldMatrix;                 // double precision, true world position
relative[3] -= glm::dvec4(m_origin, 0.0);           // subtract the camera position IN DOUBLE
const glm::mat4 model(relative);                    // only now narrow to float
```

After the subtraction, anything near the camera has *small* coordinates, and small floats are precise. The GPU never sees a big number for anything close by. The camera moves by changing its own position; it never moves the world's physical transforms. Lights, the Sun centre, trace spheres and Voyager's BVH position are all uploaded camera-relative in the same way (`LightingUniforms`, `TriangleBvh::bind`).

## 3.6 Projection and logarithmic depth

```cpp
glm::perspective(radians(fov), aspect, kNearPlane = 1e-6f, 1e6f);
```

The perspective matrix maps the viewing frustum to clip space, and the GPU divides by `w` so that far things get smaller. The problem is the **depth buffer**. With a near plane of 1e-6 and a far plane of 1e6 (twelve orders of magnitude), standard depth puts nearly all of its precision right next to the near plane. Distant surfaces then "z-fight": they flicker through each other.

**Logarithmic depth** replaces the hardware depth with a log of the distance:

```glsl
// scene.vert
logDepthW = 1.0 + gl_Position.w;          // w = distance along the view axis
// scene.frag
gl_FragDepth = log2(logDepthW) * logDepthCoefficient * 0.5;   // coefficient = 2 / log2(far + 1)
```

Each factor of 2 in distance gets the same share of depth precision, so a 5 cm antenna and a 3,000-unit orbit sort correctly in one buffer. The ray tracer writes depth with the same formula, so traced and rasterised pixels sort against each other (chapter 7).

## 3.7 Camera rigs (CameraController)

| Mode | How the camera is positioned | Enter with |
| --- | --- | --- |
| **FreeFly** | WASD/QE/Space/Ctrl move it; right-mouse or arrows turn it. Speed is `0.9 × distance to nearest surface`, clamped to 0.004..300, so it creeps near Voyager and races between planets. The wheel multiplies speed by 1.3 per notch. | Any movement key, `H` |
| **Focus** | Orbits a body's centre at a yaw/pitch/distance. The wheel zooms by ×0.85 per notch, down to 1.08 radii. | Tab, click, `[` `]`, `1`–`9` |
| **Chase** | Orbits Voyager in its own frame. During a flyby the frame turns to face the planet, which gives the classic "craft in front of the world" shot. | `C` |
| **Inspect** | Orbits one Voyager *component* in Voyager's frame, down to 0.3 of the component's size. It opens on the sunlit side. | `I`, `,` `.` |

All orbit rigs share `Camera::updateOrbit`. The camera offset in the rig's frame is `(sin yaw·cos pitch, sin pitch, −cos yaw·cos pitch) × distance`. Changing targets is a **fly-to**: position and direction are eased with `smoothStep`, and the distance is blended *logarithmically*, so a jump from Neptune to Earth starts fast and settles gently. The rig frame is smoothed with `slerp` so a sudden heading change does not jerk the view.

### Picking: clicking on a planet

`CameraController::pickAt` builds the same ray as the ray tracer's primary ray (chapter 7):

```cpp
ndc = (2x/width − 1, 1 − 2y/height);
direction = normalize(forward + ndc.x·tan(fov/2)·aspect·right + ndc.y·tan(fov/2)·up);
```

For each body, it compares the **angle** between the ray and the body's centre with the body's **angular radius**, `asin(radius / distance)`, plus a 1.2° tolerance. The nearest body that passes wins. Using angles makes tiny distant moons clickable.
