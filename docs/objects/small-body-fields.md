# Asteroid belt, Kuiper belt, Oort cloud

![Asteroid belt rocks in the foreground of a Jupiter view](images/runtime/09_focus_jupiter.jpg)

*Runtime capture: Jupiter's Focus view, with lit asteroid-belt rocks in the lower foreground.*

The low-poly sphere's exact vertices and triangles are derived in [uv-sphere.md](uv-sphere.md). The instancing mechanism is explained in [instancing.md](instancing.md).

## Overview

There are three `InstancedField` objects, `asteroid_belt`, `kuiper_belt` and `oort_cloud`. Each is one `glDrawElementsInstanced` call whatever its count, which avoids bible F9.

## Geometry generation

All three use `UvSphereGenerator::generate(6, 8)`: 63 vertices and 80 triangles. Each field uploads its own `Mesh` from that CPU data, because `Mesh::setInstanceTransforms` stores the instance buffer on the mesh.

## Vertex attributes

Standard `Vertex{position, normal, uv}` per vertex, plus a per-instance world matrix at locations 3 to 6 with `glVertexAttribDivisor(loc, 1)`. The shared `model` uniform carries only the floating-origin shift, so the shader computes `model * instanceMatrix` ([lighting.md](lighting.md)).

## Placement

Each field is centred on the Sun. Radii come from real AU bounds passed through `ScaleManager::distanceAuToRenderUnits` ([scale-manager.md](scale-manager.md)):

| Field | Shape | Real bounds | Render radii | Height jitter | Count | Rock scale | Seed |
| --- | --- | --- | --- | ---: | ---: | --- | ---: |
| Asteroid belt | annulus | 2.1-3.3 AU | 30.4-39.0 | ±0.6 | 4,000 | 0.012-0.05 | 101 |
| Kuiper belt | annulus | 30-50 AU | 131-174 | ±3 | 3,000 | 0.06-0.20 | 202 |
| Oort cloud | spherical shell | 2,000-5,000 AU | 1,323-2,189 | full 3D | 1,500 | 1.5-4.0 | 303 |

The annulus radius is `inner + (outer - inner) * sqrt(u)`, which pushes samples outward roughly in proportion to area. The shell samples `cos(phi)` uniformly and uses `radius = cbrt(inner^3 + (outer^3 - inner^3) u)`, which is exactly uniform by volume. Every rock has independent per-axis scale and a random spin axis, so the stretched spheres read as irregular rocks. The fields are deterministic because each uses a fixed seed.

## Material mapping

Flat colour, `Lit` shading. Rocks show a lit side and a dark side toward the Sun. The asteroid belt is warm grey-brown, the Kuiper belt cool grey and the Oort cloud pale icy blue.

## Limitations

- The fields are static: they do not orbit.
- Counts and rock sizes are illustrative. The real belts are mostly empty space.
- The rocks are stretched spheres, not true irregular shapes.

## Verification

1. The startup log shows `asteroid_belt: 4000 instances, 1 draw call`, and the same line for the Kuiper belt (3000) and the Oort cloud (1500).
2. Fly between Mars and Jupiter. There are lit rocks, each with a dark side facing away from the Sun.
3. Fly out past Neptune to the Kuiper belt, then zoom out with the wheel, press `Shift` and keep flying to reach the sparse Oort shell.
