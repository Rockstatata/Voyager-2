# 6. Shading techniques

[Guide index](README.md) · previous: [Lighting](05-lighting.md) · next: [Ray tracing](07-ray-tracing.md)

**Lighting** (chapter 5) is the *formula*. **Shading** is *where and how often* that formula is evaluated, and which normal it is given. The project implements five techniques. **F3** cycles through them and **Shift+F3** goes back. The value is the `shadingTechnique` uniform, defined in lighting.glsl:

```glsl
#define SHADING_FLAT 0
#define SHADING_GOURAUD 1
#define SHADING_PHONG 2
#define SHADING_BLINN_PHONG 3      // default
#define SHADING_TOON 4
```

| | Normal used | Evaluated | Specular | Maps |
| --- | --- | --- | --- | --- |
| Flat | one per triangle | per fragment | Blinn | specular map only |
| Gouraud | per vertex | **per vertex**, colour interpolated | Phong | none |
| Phong | interpolated, per pixel | per fragment | Phong (reflect) | yes |
| Blinn-Phong | interpolated, per pixel | per fragment | Blinn (half vector) | yes |
| Toon | interpolated, per pixel | per fragment | Blinn, thresholded | yes |

## Earth

| Flat | Gouraud | Phong |
| --- | --- | --- |
| ![](../objects/images/shading/earth_flat.jpg) | ![](../objects/images/shading/earth_gouraud.jpg) | ![](../objects/images/shading/earth_phong.jpg) |

| Blinn-Phong | Toon |
| --- | --- |
| ![](../objects/images/shading/earth_blinn_phong.jpg) | ![](../objects/images/shading/earth_toon.jpg) |

## Voyager 2

| Flat | Gouraud | Phong |
| --- | --- | --- |
| ![](../objects/images/shading/voyager_flat.jpg) | ![](../objects/images/shading/voyager_gouraud.jpg) | ![](../objects/images/shading/voyager_phong.jpg) |

| Blinn-Phong | Toon |
| --- | --- |
| ![](../objects/images/shading/voyager_blinn_phong.jpg) | ![](../objects/images/shading/voyager_toon.jpg) |

## 6.1 Flat shading

One normal per triangle, so each triangle is a single uniform shade and the facets are visible. You would normally store a face normal per vertex, but that needs different vertex data. Instead the fragment shader *computes* the face normal:

```glsl
vec3 faceNormal = normalize(cross(dFdx(relativePosition), dFdy(relativePosition)));
surfaceNormal = dot(faceNormal, surfaceNormal) < 0.0 ? -faceNormal : faceNormal;
```

`dFdx(p)` is how much the position changes from this pixel to its right neighbour, and `dFdy(p)` to the pixel above. Both vectors lie **in the triangle's plane** (the GPU shades pixels in 2×2 blocks that belong to the same triangle), so their cross product is the triangle's normal. The sign is fixed to agree with the interpolated normal, because the cross product's direction depends on screen orientation.

*Use it to see the mesh.* The 64×32 sphere's latitude bands and Voyager's decagonal bus show clearly.

## 6.2 Gouraud shading

Lighting is evaluated **once per vertex** in `scene.vert`. The resulting colours (four `LightTerms` vectors) are passed to the fragment shader, and the rasteriser interpolates them across the triangle:

```glsl
// scene.vert
if (shadingTechnique == SHADING_GOURAUD && (shadingModel == 1 || shadingModel == 2))
{
    LightTerms terms = evaluateLights(n, v, relative.xyz, twoSided, SHADING_GOURAUD, ...);
    gouraudSunDiffuse = terms.sunDiffuse;   ... (out varyings)
}
// scene.frag
terms.sunDiffuse = gouraudSunDiffuse;       ... (interpolated)
```

It is cheap, because there are far fewer vertices than pixels. Its well-known weakness shows up here: **a specular highlight smaller than a triangle disappears or smears**, because the highlight only exists where a vertex happens to catch it. Look at Earth's ocean glint in the Gouraud image, and at Voyager's lens and foil. Normal maps cannot work per vertex, so they are off in Gouraud.

## 6.3 Phong shading

The normal is **interpolated** across the triangle (the `normal` varying), then re-normalised per pixel, and the full lighting runs per pixel with the **reflect**-based specular (chapter 5.2). The highlight is round and sharp at any size, and normal maps work. Do not confuse the Phong *reflection model* (ambient + diffuse + specular) with Phong *shading* (per-pixel interpolation of normals). This technique uses both.

## 6.4 Blinn-Phong shading (default)

The same per-pixel pipeline as Phong, but the specular uses the **half vector** `h = normalize(l + v)` with `pow(dot(n, h), 2 × power)`. The highlight is slightly more elongated at grazing angles (more realistic, and closer to real foil and water), and it is the standard in real-time graphics. It is also what the ray tracer uses.

## 6.5 Toon (cel) shading

A non-photorealistic style made from three changes:

1. **Diffuse bands.** The smooth ramp is quantised into four flat steps:

```glsl
float toonBand(float lambert)
{
    if (lambert > 0.75) return 1.0;
    if (lambert > 0.40) return 0.62;
    if (lambert > 0.12) return 0.30;
    return 0.06;
}
```

2. **Hard highlight**: `specular = step(0.5, specular)`, so the highlight is either full or nothing.
3. **Ink outline** (in scene.frag): where the surface turns away from the eye (`dot(n, v) < 0.22`), the colour is multiplied by 0.15, which draws a dark rim along every silhouette.

## 6.6 Where each piece lives

| Want to change... | Edit |
| --- | --- |
| The lighting formula for every technique | `evaluateLights` in shaders/lighting.glsl |
| The band thresholds or colours in toon | `toonBand` in lighting.glsl |
| The outline thickness | the `0.22` in scene.frag |
| Which normal each technique uses | scene.frag, the block starting `if (shadingTechnique == SHADING_FLAT)` |
| The key or the default technique | `LightingController::handleKeys`, and `m_technique` in LightingController.h |
| Add a sixth technique | chapter 10, recipe "Add a shading technique" |

`scripts/verify_navigation_and_motion.ps1` checks that all five `SHADING_*` defines and all three `LIGHT_*` defines still exist.
