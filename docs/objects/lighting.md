# Sun lighting, glow and depth

![Jupiter lit by the Sun, day side toward the camera](images/runtime/09_focus_jupiter.jpg)

*Runtime capture: Focus on Jupiter. The Lambert terminator runs down the left limb and the faint main ring is translucent. The moons are lit by the same point light.*

Bible Phase 14 needed a lighting model that plugs into the existing `Material` → `Renderer` → shader seam without per-object shader code (bible section 47). This page documents that model, the halo shells, and the depth changes that let one frame show a 2 cm antenna strut and the 3,000-unit Oort cloud together. Code: `default.vert`, `default.frag`, `src/rendering/Renderer.*`, `src/rendering/Material.h`.

## Uniform interface

| Uniform | Set by | Meaning |
| --- | --- | --- |
| `model`, `view`, `proj` | Renderer | Camera-relative model matrix, view at the origin, perspective |
| `baseColor`, `useTexture`, `albedoTexture` | Material | Unchanged Phase 2 albedo |
| `shadingModel` | `Material::shading` | 0 Unlit, 1 Lit, 2 LitTwoSided, 3 Glow |
| `specularStrength`, `specularPower` | Material | Blinn-Phong highlight; for Glow, `specularPower` is the falloff exponent |
| `opacity` | Material | Below 1, the draw is deferred and alpha blended |
| `lightPosition`, `lightColor` | `Renderer::setLight` | The Sun, camera-relative |
| `lightingEnabled` | `K` key | 0 shows raw albedo everywhere |
| `logDepthCoefficient` | Renderer | `2 / log2(far + 1)` |

The `Vertex` layout and the shared sphere are unchanged. Lighting uses only the `normal` attribute, which has been in every mesh since Phase 2.

## Lit surfaces

For each fragment at camera-relative position `p` with normal `n`:

```text
l = normalize(lightPosition - p)             direction to the Sun
v = normalize(-p)                            direction to the eye
diffuse  = max(dot(n, l), 0)
specular = specularStrength * max(dot(n, normalize(l + v)), 0)^specularPower   (lit side only)
colour   = albedo * (0.07 + diffuse * lightColor) + specular * lightColor
```

- The light does not fall off with distance. At real brightness Neptune would receive 1/900 of Earth's sunlight and appear black, so the scene uses constant illumination. The terminator position (day or night side) is still physically correct.
- Ambient light is 0.07, so night sides are dark but still show their outline.
- Specular strength is 0.18 for Earth (ocean glint), 0.05 for the other planets and 0.35 for Voyager's painted and foil parts.
- `LitTwoSided` rings use `abs(dot(n, l))` and flip the normal toward the viewer, so a ring seen from the unlit side is not black. Real rings are translucent and scatter light through.
- `Unlit` is used by the Sun's own surface, orbit guides, the trajectory, heliosphere circles, stars and the comet tail. These are light sources or abstract guides, not surfaces.

## Glow shells

The Sun has two child spheres drawn with `ShadingModel::Glow`: `sun_corona` (1.25 radii, falloff exponent 1.5, opacity 0.9) and `sun_halo` (2.6 radii, exponent 3, opacity 0.55). The comet's coma uses the same model. With `facing = |dot(n, v)|`:

```text
alpha = facing^specularPower * opacity         additive: src * alpha + dst
```

The shell is brightest where the eye looks through its thickest part, just outside the solar disc, and fades to nothing at its silhouette. Glow and translucent draws are queued by `Renderer::submit` and drawn in `endFrame` after all opaque geometry. They are depth-tested but do not write depth, so a planet in front hides the halo but the halo never hides anything.

## Floating origin

`Renderer::beginFrame` stores the camera position. `submit` receives each world matrix in double precision, subtracts that position from the translation column, and only then narrows to float. The GPU sees only camera-relative coordinates, which never exceed a few hundred units. That keeps Voyager's 1 cm struts stable 170 units from the Sun (bible section 20). Instanced fields hold world matrices, so their shared `model` uniform is `translate(-cameraPosition)` and the shader computes `model * instanceMatrix`.

## Logarithmic depth

The projection uses near 1e-6 and far 1e6 units. The fragment shader replaces hardware depth with

```text
gl_FragDepth = log2(1 + w) * logDepthCoefficient * 0.5
```

Depth precision is then roughly constant relative to distance. The chase camera can sit 7 cm from Voyager while Saturn's rings, the heliopause and the Oort cloud stay correctly ordered behind it. No camera mode changes the near or far plane.

## Background

Three star layers of 5,200, 1,800 and 380 points at increasing brightness are drawn first by `Renderer::submitBackground`, with depth writes off and centred on the camera ([starfield.md](starfield.md)).

## Limitations

- There are no shadows: rings are not shadowed by their planet, and moons do not cast eclipse shadows.
- Lighting is per fragment with a single light and no atmosphere scattering, bloom, normal maps or HDR.
- Constant solar intensity is a deliberate readability choice.

## Verification

1. Press `Tab` to focus Earth. The HUD reads `CAMERA FOCUS: EARTH`, the day side faces the camera, the terminator is soft, and there is a highlight on the ocean.
2. Press `K`. Every body switches to flat albedo, and the log reports `[APP] Sun lighting off`. Press `K` again to restore lighting.
3. Fly round Saturn with `C` and `WASD`. The rings stay visible from both sides, and the Cassini Division shows stars through it.
4. Zoom the chase camera in with the wheel to about 1.3 bounding radii. The antenna struts do not shimmer or z-fight against the dish.
