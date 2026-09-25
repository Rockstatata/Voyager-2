# 5. Lighting

[Guide index](README.md) · previous: [Textures](04-textures.md) · next: [Shading techniques](06-shading-techniques.md)

Every lit pixel in the project is computed by **one function**, `evaluateLights` in `shaders/lighting.glsl`. The raster vertex shader (Gouraud), the raster fragment shader (every other technique) and the ray tracer all `#include` it. Change the light model there and every path changes together.

## 5.1 The three terms (Phong reflection model)

A surface's colour under a light is split into three parts:

```text
colour = albedo × (ambient + diffuse) + specular
```

| Term | Physical idea | Formula | Depends on the viewer? |
| --- | --- | --- | --- |
| **Ambient** | Light that has bounced around so much it comes from everywhere | constant `ambientStrength = 0.07` | no |
| **Diffuse** (Lambert) | Matte surfaces scatter light equally in all directions. A surface tilted away from the light catches less of it. | `max(dot(n, l), 0)` | no |
| **Specular** | Shiny surfaces mirror the light source as a highlight | `pow(max(dot(v, r), 0), shininess) × strength` | **yes** |

The vectors, all unit length and pointing *away* from the surface point:

```text
        l (to light)    n (normal)    v (to eye)
             \             |             /
              \            |            /
               \           |           /
       ─────────────────── p ───────────────────  surface
r = reflect(−l, n): the mirror direction of the light about the normal
```

- **Why `dot(n, l)`?** A beam of light of fixed width spreads over a larger area when it hits a surface at a slant. The energy per unit area is proportional to cos(angle) = n·l (Lambert's cosine law). At noon on Earth's equator n·l ≈ 1; at the terminator (the day/night line) it is 0.
- **Why the max with 0?** A negative value means the light is behind the surface: no light, not negative light.
- **Why the albedo multiplies only ambient + diffuse?** Diffuse light has soaked into the material and taken on its colour. The specular highlight is reflected off the surface, so it keeps the light's colour. That is why the sun glint on Earth's ocean is white, not blue.

## 5.2 Specular: Phong vs Blinn-Phong

```glsl
if (technique == SHADING_PHONG || technique == SHADING_GOURAUD)
{
    vec3 r = reflect(-l, n);                          // mirror direction
    specular = pow(max(dot(v, r), 0.0), specularPower);
}
else
{
    vec3 h = normalize(l + v);                        // half vector: halfway between light and eye
    specular = pow(max(dot(n, h), 0.0), specularPower * 2.0);
}
specular *= specularStrength;
```

- **Phong** measures how close the eye is to the perfect mirror ray.
- **Blinn-Phong** measures how close the normal is to the **half vector** h. When n = h, the surface is angled exactly to bounce light into the eye. It is cheaper (no reflect) and behaves better at grazing angles, where Phong's `dot(v, r)` goes negative and the highlight is cut off sharply.
- The angle between n and h is about half the angle between v and r, so Blinn needs about **2× the exponent** to give the same highlight size. The code doubles it so that switching technique compares like with like.
- `specularPower` (the shininess) sets the highlight's *size*: 8 is broad and dull, 120 is a pin-point like a camera lens. `specularStrength` sets its *brightness*.

Specular is only added when `lambert > 0`, so no highlight appears on the unlit side.

## 5.3 Light types (light casters)

`LightType` in `src/rendering/Lighting.h`: Directional = 0, Point = 1, Spot = 2. The GLSL `Light` struct:

```glsl
struct Light
{
    int type; int enabled;
    vec3 position;       // point and spot (camera-relative)
    vec3 direction;      // directional and spot: the way the light TRAVELS
    vec3 color;          // already multiplied by intensity
    vec3 attenuation;    // constant, linear, quadratic
    float innerCutoff;   // spot: cosines of the cone angles
    float outerCutoff;
};
```

### Directional light: the fill

The light is infinitely far away, so every ray is parallel and there is no falloff: `l = normalize(−direction)`. The project's **fill light** (F6) is a dim, cool blue directional light from ecliptic north (0.25, −1, 0.15). It stops night sides and Voyager's far faces from being pure black. In Inspect mode an automatic **studio fill** comes from behind the camera instead, like a photographer's softbox.

### Point light: the Sun

The light spreads out from a point. `l = normalize(lightPosition − p)`, and the brightness falls with distance d:

```glsl
attenuation = 1.0 / (k.x + k.y·d + k.z·d²);     // constant, linear, quadratic
```

Real light follows the inverse-square law (constant 0, linear 0, quadratic 1). The project compresses distances with a power law (Neptune is about 132 units from the Sun, only 6.6× Earth's 20, instead of 30×). Pure 1/d² would make Neptune 1/900 as bright as Earth, which is invisible. The Sun uses (F7):

```text
off (default):  (1, 0, 0)               -> every planet fully lit (best for looking at textures)
on:             (0.3, 0, 0.7 / 20²)     -> 1.0 at Earth's 20-unit orbit, ~0.03 at Neptune
```

The Sun is **light 0**, the only light that casts shadows (chapter 7.3). `LightTerms` keeps its diffuse and specular separate from the other lights so that only the Sun's part is multiplied by the shadow factor:

```glsl
vec3 diffuse  = terms.sunDiffuse  * sunlight + terms.otherDiffuse;
vec3 specular = terms.sunSpecular * sunlight + terms.otherSpecular;
vec3 lit = color * (ambientStrength + diffuse) + specular;
```

### Spotlight: the camera headlamp

A point light limited to a cone. θ is the angle between the spot's axis and the direction to the pixel. Between the **inner** cone (12°, full brightness) and the **outer** cone (18°, zero) it fades linearly, which gives a soft edge:

```glsl
float theta = dot(-l, normalize(lights[i].direction));          // cos of the angle off-axis
float edge  = innerCutoff - outerCutoff;                         // both are cosines
attenuation *= clamp((theta - outerCutoff) / edge, 0.0, 1.0);
```

Cosines are compared instead of angles because the dot product gives the cosine directly, with no `acos` per pixel. The headlamp (F5) sits at the eye and points where you look. Its quadratic falloff is scaled to 3× the distance to the nearest surface, so it works equally well beside Voyager and beside Jupiter.

![Headlamp](../objects/images/shading/light_headlamp.jpg)

## 5.4 Materials: how a surface answers the light

`Material` (src/rendering/Material.h) is the **only** way to change how something looks. There is no per-object shader code.

| Field | Meaning |
| --- | --- |
| `baseColor` | multiplies the texture (a tint), or is the colour when there is no texture |
| `albedoTexture` | the photograph |
| `shading` | `Unlit` (Sun, lines, stars), `Lit`, `LitTwoSided` (rings: lit from whichever face sees the Sun), `Glow` (additive halo) |
| `specularStrength`, `specularPower` | highlight brightness and tightness |
| `opacity` | < 1 is drawn in the translucent pass |
| `normalTexture`, `normalStrength` | bump detail (chapter 4.5) |
| `specularTexture` | per-texel highlight mask |
| `uvTransform` | atlas window (chapter 4.4) |
| `selfShadowing` | trace shadow rays through Voyager's own triangles (chapter 7.7) |

Planet presets (`MaterialLibrary::surface`, chosen by the `material` column of celestial_bodies.csv):

| Preset | Specular strength / power | Maps | Bodies |
| --- | --- | --- | --- |
| emissive | unlit | none | Sun |
| rocky | 0.03 / 8 (almost matte) | normal 2.0 | Mercury, Mars, Moon, Io, Callisto |
| ocean | 0.55 / 48 (sun glint) | normal 1.2 + ocean mask | Earth |
| ice | 0.22 / 36 | normal 1.6 | Pluto, Europa, Ganymede, the Saturn and Uranus moons, Triton |
| gas | 0.06 / 10 (soft sheen) | none | Jupiter, Saturn, Uranus, Neptune |
| cloud | 0.10 / 6 (broad haze) | none | Venus, Titan |

## 5.5 The lighting rig (LightingController)

`LightingController::build` assembles a `LightingState` every frame. The Sun follows the simulated Sun position, the headlamp follows the camera, and the fill follows the rules above. `LightingUniforms::uploadLights` subtracts the camera position in double from each light position before sending it (floating origin), and pre-multiplies colour by intensity.

| Key | Toggle | Default |
| --- | --- | --- |
| K | all lighting on/off (off = raw texture colours) | on |
| F3 / Shift+F3 | shading technique (chapter 6) | Blinn-Phong |
| F4 | shadows: off, hard, soft | soft |
| F5 | headlamp spotlight | off |
| F6 | fill directional light | off |
| F7 | Sun distance falloff | off |
| F8 | normal and specular maps | on |

The top-left HUD panel shows the current state.

## 5.6 Two-sided and special shading models

- **Rings** (`LitTwoSided`): a thin sheet has no "inside". The normal is flipped to face the viewer, and `lambert = abs(dot(n, l))`, so the sheet is lit from whichever side the Sun is on.
- **Glow** (`shadingModel 3`): the Sun's halo shell. Alpha = `pow(|dot(n, v)|, specularPower) × opacity`: solid toward the middle of the shell, fading to nothing at its edge. It is drawn additively (`GL_ONE`) after everything else.
- **Unlit**: `color = albedo × baseColor`. The Sun is the light source itself, so lighting it would be meaningless.
