# Reference-Only Visual Research

This record names visual references consulted during planning. It is **not** an asset manifest: no image, texture, model, pixels, or decoded derivative from these sources is bundled or loaded by the program.

## Permitted Use in This Project

Use public science imagery only to learn high-level visual characteristics, then implement original geometry and original procedural or hand-authored textures. Do not trace coastlines, copy pixels, sample colours programmatically, or distribute a downloaded reference as a runtime asset without explicit instructor permission.

The Phase 2 executable now uses only `ProceduralTextureGenerator`; its recipes are documented in [`assets/textures/phase2/README.md`](../../assets/textures/phase2/README.md).

## Reference Topics

| Body | What may be studied | Reference |
| --- | --- | --- |
| Earth | broad ocean/land/ice/cloud visual categories; not coastlines or a map | [NASA Blue Marble overview](https://science.nasa.gov/earth/earth-observatory/the-blue-marble-true-color-global-imagery-at-1km-resolution/) |
| Jupiter | broad horizontal atmospheric banding and the idea of large oval storms; not a copied cloud map | [NASA/JPL Cassini Jupiter overview](https://science.nasa.gov/photojournal/cassinis-best-maps-of-jupiter-cylindrical-map/) |
| Sun | the idea of turbulent, high-contrast solar surface patterns; not a scientific texture or colour measurement | [NASA SVS solar visualization](https://svs.gsfc.nasa.gov/3851) |

For a teacher demonstration, describe these as reference study, then point to the exact original equations and seeds in `src/rendering/ProceduralTextureGenerator.cpp`. Do not present the stylized output as a NASA product or an accurate geographical/astronomical dataset.
