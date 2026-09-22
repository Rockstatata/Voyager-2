# Asteroid Belt, Kuiper Belt, Oort Cloud

The low-poly sphere's exact vertices and triangles are derived in [uv-sphere.md](uv-sphere.md), while the generator-to-object map is summarized in the [Procedural Mesh Construction Handbook](procedural-meshes.md).

## Overview

Three `InstancedField` objects (`asteroid_belt`, `kuiper_belt`, `oort_cloud`). Bible sections 27/28: "use the same principle as the asteroid belt" for the Kuiper belt, and don't let it dominate performance — all three share one mechanism, fully explained in [instancing.md](instancing.md). This document covers only what's specific to each field: where it sits and how many bodies it has.

## Geometry generation

All three share one CPU shape: `UvSphereGenerator::generate(6, 8)` — a deliberately low-poly sphere (63 vertices, 80 triangles) for a rock that's tiny and usually distant on screen; the full 32x64 sphere used for planets would waste GPU work nobody can see the benefit of, per the triangle-budget reasoning in [uv-sphere.md](uv-sphere.md). Each field uploads its **own** `Mesh` from this same CPU data — three small GPU uploads, not one shared `Mesh` — because `Mesh::setInstanceTransforms` stores the instance buffer on the `Mesh` object itself; sharing one `Mesh` across fields would mean the second field's `setInstanceTransforms` call overwrites the first field's instances.

## Vertex attributes / triangle construction

Standard `Vertex{position, normal, uv}`, same as every sphere in this project — see [uv-sphere.md](uv-sphere.md). Per-instance placement (not per-vertex) is covered in [instancing.md](instancing.md).

## Transform / placement

All three fields are centered on the Sun's current world position (so they move together if the Sun's placement ever changes). Real placement reasoning: the actual asteroid belt sits between Mars and Jupiter, the Kuiper belt beyond Neptune — radii are tied to `ScaleManager`'s log-compressed planet distances (`scale-manager.md`): Mars ≈ 5.44, Jupiter ≈ 8.97, Neptune = 14.0 render units:

| Field | Shape | Inner radius | Outer radius | Height jitter | Count | Seed |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| Asteroid belt | flat annulus | 5.9 | 8.5 (between Mars ≈5.44 and Jupiter ≈8.97) | ±0.15 | 2,500 | 101 |
| Kuiper belt | flat annulus | 14.8 | 18.0 (beyond Neptune = 14.0) | ±0.20 | 1,500 | 202 |
| Oort cloud | spherical shell | 25.0 | 40.0 | n/a (full 3D) | 800 | 303 |

Per-instance scale ranges: asteroid belt 0.006-0.020, Kuiper belt 0.008-0.024, Oort cloud 0.010-0.030 (render units, independent per axis — see instancing.md).

## Material mapping

Flat color per field, no texture: asteroid belt warm grey-brown, Kuiper belt cool grey, Oort cloud pale blue-white (icy). No lighting — consistent with the rest of this pass.

## Limitations

- Placement radii are pinned to `ScaleManager`'s current calibration constants, not literal AU distances.
- Fixed distributions computed once at startup; the fields themselves don't orbit (unlike planets/moons — see `orbital-motion.md`), matching the real asteroid/Kuiper belts' comparatively slow bulk drift versus a single body's orbit.
- "Irregular shape" is a stretched sphere, not real irregular geometry.
- Oort cloud radius (20-32) and count (800) are illustrative, not derived from the real Oort cloud's actual (enormously larger, ~2,000-100,000 AU) scale — at real relative scale it would be far outside this scene entirely.

## Verification

- `[SCENE] asteroid_belt: 2500 instances, 1 draw call` (and the same for the other two) at startup.
- Visual: fly between Mars and Jupiter and confirm scattered small rocks, not a gap; fly beyond Neptune for the Kuiper belt; fly far out for the sparse Oort shell.
