# Spacecraft textures

| File | Used by | Resolution | Source |
| --- | --- | ---: | --- |
| `voyager_nasa_atlas.png` | Voyager 2 surface finishes | 1024x1024 | NASA 3D Resources, public domain |

**Source.** The image embedded in `3D Models/Voyager Probe (B)/Voyager Probe (B).glb` in the NASA 3D Resources repository (<https://github.com/nasa/NASA-3D-Resources>). The repository's README states that its models and textures are free to use without copyright. It is a texture atlas of photographs of real Voyager flight hardware: black and gold multi-layer insulation blankets, the Golden Record, thermal louvres, the optical calibration target, striped radiator panels, lens apertures and metal struts.

**How it was extracted.** A GLB file is a 12-byte header, then a JSON chunk, then a binary chunk. The JSON's `images[k].bufferView` points to a byte range of the binary chunk, which here is a PNG. Only that PNG was copied out. The GLB's meshes, vertices, UVs and materials were **not** used: Voyager's geometry is built entirely by `VoyagerModelBuilder` (project texture policy, CLAUDE.md).

**How it is applied.** Every part keeps the UVs of the project's own generators: box faces, cylinder sides and caps, and the dish all run 0 to 1. A material's `uvTransform` then squeezes that 0-to-1 square into one named rectangle of the atlas (`scene.vert`: `uv * scale + offset`). See [docs/objects/voyager-textures.md](../../../docs/objects/voyager-textures.md) for the region table.
