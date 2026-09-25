# 4. Textures

[Guide index](README.md) · previous: [Transforms and cameras](03-transforms-and-cameras.md) · next: [Lighting](05-lighting.md)

Geometry decides *shape*. Textures decide *colour detail* and only affect the fragment stage. The project's rule: **geometry is always self-authored; textures may be real, credited photographs.** The body maps are the Solar System Scope 2k pack (CC BY 4.0). The spacecraft atlas comes from NASA 3D Resources (public domain). Every file is listed in [assets/textures/bodies/README.md](../../assets/textures/bodies/README.md) and [assets/textures/spacecraft/README.md](../../assets/textures/spacecraft/README.md).

## 4.1 From file to GPU (Texture2D)

```cpp
bool Texture2D::decodeFile(path, width, height, pixels)
{
    stbi_set_flip_vertically_on_load(true);            // see below
    unsigned char* decoded = stbi_load(path, &width, &height, &channels, 4);   // force RGBA
    pixels.assign(decoded, decoded + width * height * 4);
    stbi_image_free(decoded);
}

bool Texture2D::uploadRgba(width, height, pixels, name)
{
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);          // u wraps around the planet
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);   // v stops at the poles
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);                                  // rows are tightly packed
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
}
```

- **Why flip?** Image files store the *top* row first. OpenGL's texel (0,0) is the *bottom-left*. After the flip, v = 1 is the top of the picture (the north pole of a planet map), which matches the sphere's `v = 1 − latitudeFraction` (chapter 2.2).
- **Wrap modes.** `REPEAT` on u lets longitude wrap. `CLAMP_TO_EDGE` on v stops the south-pole row from bleeding into the north pole.
- **Mipmaps** are pre-shrunk copies of the image (½, ¼, ⅛ ...). A distant planet covers only a few pixels. Without mipmaps it would sample a few random texels from a 2048-wide map and sparkle. `LINEAR_MIPMAP_LINEAR` blends between the two nearest mip levels (trilinear filtering).
- `Texture2D` is a move-only RAII owner: its destructor calls `glDeleteTextures`.

## 4.2 Sampling in the shader

```glsl
texCoord = aTexCoord * uvTransform.zw + uvTransform.xy;              // scene.vert (4.4)
vec4 albedo = useTexture != 0 ? texture(albedoTexture, texCoord) : vec4(1.0);   // scene.frag
vec3 color  = albedo.rgb * baseColor;                                  // tint
```

`Texture2D::bind(unit)` does `glActiveTexture(GL_TEXTURE0 + unit)` and then binds the texture. The sampler uniform is set to that unit number. Fixed unit plan:

| Unit | Sampler | Used by |
| --- | --- | --- |
| 0 | `albedoTexture` (raster), `albedoAtlas` (ray tracer) | colour |
| 1 | `normalMap` | bump detail |
| 2 | `specularMap` | per-texel shininess |
| 3, 4 | `bvhNodes`, `bvhTriangles` (texture buffers) | Voyager ray tracing |
| 5 | `meshAtlas` | Voyager colour inside the ray tracer |

## 4.3 How UVs put a photo on a sphere

The body maps are **equirectangular**: x is proportional to longitude and y to latitude. The UV sphere's `u = 1 − θ/2π`, `v = 1 − φ/π` is exactly that projection, so every vertex samples the pixel at its own latitude and longitude. The duplicated seam column (chapter 2.2) gives the wrap its u = 0 and u = 1 vertices. The texels are squeezed toward the poles, which is the known limit of equirectangular maps and why pole detail is soft.

## 4.4 Texture atlases (Voyager)

NASA's Voyager model comes with **one 1024×1024 image** containing photographs of many kinds of hardware: gold foil, white paint, black blankets, aluminium, louvres, the Golden Record, the calibration target. This is an *atlas*. We keep our own geometry and use only the image. Each Voyager material shows **one rectangle** of it.

The rectangle is given in atlas **pixels** (left, top, right, bottom), which you can read straight off the image in any paint program. `MaterialLibrary::spacecraftTextured` converts it to a UV window:

```cpp
const float u0 = rect.x / width;              // left
const float u1 = rect.z / width;              // right
const float v0 = 1 - rect.w / height;         // bottom pixel edge -> small v   (image was flipped on load)
const float v1 = 1 - rect.y / height;         // top pixel edge    -> large v
material->uvTransform = vec4(u0, v0, u1 - u0, v1 - v0);   // offset.xy, scale.zw
```

The vertex shader maps the mesh's own 0..1 UVs into that window: `uv * scale + offset`. One texture bind serves every spacecraft part. The rectangles used (VoyagerModelBuilder.cpp):

| Finish | Pixels (l, t, r, b) | Tint | Specular / power | Parts |
| --- | --- | --- | --- | --- |
| whitePaint | 225, 835, 352, 958 | 1.0 | 0.20 / 16 | dish, subreflector, cameras, magnetometers |
| goldFoil | 390, 710, 490, 818 | 1.15 | 0.75 / 70 | bay blankets, plasma science, cosmic ray, IRIS |
| darkGoldFoil | same | 0.62 | 0.55 / 50 | bays, canister, LECP, scan platform |
| blackBlanket | 8, 8, 212, 160 | 1.0 | 0.12 / 10 | bays, UVS |
| aluminium | 978, 20, 1010, 560 | 1.1 | 0.55 / 40 | trusses, ribs, struts |
| darkMetal | 150, 492, 232, 626 | 0.45 | 0.35 / 30 | bus frame, RTGs, feeds, thruster blocks |
| lens | 298, 672, 360, 736 | 1.0 | 0.90 / 120 | camera lenses, cups, apertures |
| radiatorBlue | 480, 356, 640, 536 | 1.0 | 0.30 / 24 | shunt radiator |
| recordGold | 8, 172, 234, 396 | 1.1 | 0.85 / 90 | Golden Record |
| louvres | 278, 122, 448, 280 | 1.0 | 0.60 / 50 | thermal louvres |
| calibrationPanel | 470, 20, 630, 220 | 1.0 | 0.15 / 12 | calibration target |
| copper | untextured (0.72, 0.36, 0.12) | n/a | 0.60 / 48 | thruster nozzles |

Details: [voyager-textures.md](../objects/voyager-textures.md).

## 4.5 Lighting maps derived from the photo (SurfaceMaps)

The project has no hand-painted bump maps, so it **derives** them from the colour photograph on the CPU at load time.

### Normal map (Sobel on luminance)

1. Height ≈ brightness: `h = 0.2126 R + 0.7152 G + 0.0722 B`. Crater rims are bright and floors dark, a fair guess for regolith.
2. Gradient with the **Sobel** operator, a smoothed 3×3 central difference, so JPEG noise does not turn into sparkle:

```text
dx = [h(x+1,y−1) + 2h(x+1,y) + h(x+1,y+1)] − [h(x−1,y−1) + 2h(x−1,y) + h(x−1,y+1)]
dy = the same vertically
normal = normalize(−dx·strength, −dy·strength, 1)
```

3. Encode −1..1 into a colour 0..255 with `c = n·0.5 + 0.5`. That is why normal maps look lilac.

Longitude wraps (`x % width`) so the seam has no crease, and latitude clamps at the poles. Relief strength per preset: rocky 2.0, ice 1.6, ocean 1.2, spacecraft atlas 1.4 (`loadAtlas`).

### Specular map (Earth's oceans)

`oceanSpecularMap` marks a texel as water if it is blue-dominant and not bright (`b > 1.25r`, `b > 1.02g`, `r+g+b < 1.5`). Water gets 255 and land 30. The shader multiplies the highlight by it, so the sun glints on the Pacific but not on the Sahara.

### Using the normal map without tangents

A normal map stores directions in **tangent space**: x along u, y along v, z out of the surface. To use it, the shader needs the tangent (T) and bitangent (B) at each pixel. Our `Vertex` has no tangent attribute, and adding one would change the shared format. Instead `perturbNormal` in scene.frag rebuilds T and B **per pixel from screen-space derivatives** (Schüler 2006):

```glsl
vec3 dp1 = dFdx(p);  vec3 dp2 = dFdy(p);          // how position changes to the next pixel
vec2 duv1 = dFdx(uv); vec2 duv2 = dFdy(uv);       // how UV changes to the next pixel
vec3 tangent   = cross(dp2, n) * duv1.x + cross(n, dp1) * duv2.x;
vec3 bitangent = cross(dp2, n) * duv1.y + cross(n, dp1) * duv2.y;
mat3 tbn = mat3(tangent * s, bitangent * s, n);   // s normalises the frame's scale
vec3 mapped = texture(normalMap, uv).xyz * 2.0 - 1.0;
mapped.xy *= normalStrength;
return normalize(tbn * mapped);
```

Compare the Moon with maps off and on (F8):

| Maps off | Maps on |
| --- | --- |
| ![](../objects/images/shading/maps_moon_off.jpg) | ![](../objects/images/shading/maps_moon_on.jpg) |
| ![](../objects/images/shading/maps_earth_off.jpg) | ![](../objects/images/shading/maps_earth_on.jpg) |

## 4.6 Texture arrays (the ray tracer)

The ray tracer shades every planet in **one** draw, so it cannot bind a different texture per body. `RayTracer::buildAtlas` resamples every body map to **1024×512** with bilinear filtering and stacks them as the layers of a `GL_TEXTURE_2D_ARRAY`. The shader then picks a layer by index: `textureGrad(albedoAtlas, vec3(uv, layer), dx, dy)`. It is built lazily on the first traced frame, so startup is not slowed. The explicit gradients remove the u = 0/1 jump, which would otherwise select the tiniest mip level and draw a blurred seam line. Chapter 7.5 has the details.
