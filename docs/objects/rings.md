# Planetary Rings (Saturn and Uranus)

## Current visible scope

Only Saturn and Uranus receive visible ring geometry. Jupiter's and Neptune's real dust rings are too faint and thin for this unlit, flat-color presentation; opaque annuli were misleadingly prominent, so those two systems are intentionally omitted.

The exact annulus vertex equations, index order, normals, winding, and count formula are centralized in the [Procedural Mesh Construction Handbook](procedural-meshes.md). This page records the object-specific band radii, transforms, colors, and limitations.

## Overview

All four ring-bearing planets from bible section 26 ("Jupiter, Saturn, Uranus, and Neptune need ring support ... with Saturn's rings most developed") now have bands: Saturn and Uranus first (the two visually obvious ones), Jupiter and Neptune added the same session once the mechanism was verified working — both are real but famously faint dust rings, included for completeness rather than visual impact. Every band is a plain `SceneObject` child of its planet's `CelestialBody`.

## Geometry generation

`RingGenerator::generate(innerRadius, outerRadius, radialSegments)` (`src/rendering/RingGenerator.*`) builds a flat annulus in the local XZ plane (`y = 0`): two concentric rings of vertices (inner, outer), connected into quads (2 triangles each). Built **double-sided** — the whole ring is generated twice, once with `normal = (0,1,0)` and CCW-from-above winding, once with `normal = (0,-1,0)` and reversed winding — because the free-fly camera can pass underneath the ring plane and this renderer has no per-mesh culling toggle; a single-sided ring would vanish from below.

The key design choice: **radii are given in units of the parent planet's own radius**, the same convention `UvSphereGenerator` uses for its unit sphere. A ring is added as a plain child with no explicit scale of its own — `worldMatrix = planet.worldMatrix() * ring.localMatrix()` already includes the planet's `transform().scale`, so the ring's rendered size automatically stays proportional to however large that planet is currently drawn (today's Phase-3 placeholder scale, or whatever Phase 4's `ScaleManager` computes later) without RingGenerator ever needing to know the planet's absolute render radius.

**Each named ring is its own `RingGenerator::generate()` call and its own child `SceneObject`**, not one annulus. Two reasons: real ring systems have gaps (the Cassini Division has no ring material in it at all — that has to be an actual geometric gap, not a color change), and this codebase has no per-vertex color yet (`Vertex` is `position, normal, uv` only — see `phase-2-rendering-pipeline.md`), only per-`Material` flat color, so distinct bands need to be distinct objects to get distinct shades.

## Vertex attributes / triangle construction

Same `Vertex{position, normal, uv}` layout as every other object in the scene.

## Transform and real-world basis

Radii (source: standard ring-system references, in km, divided by the planet's own real radius from its own `CelestialBodyData`). Each row is a separate band/`SceneObject`; a gap between two rows with no band is real (no geometry generated there), not a rendering artifact:

**Saturn** (radius 58,232 km) — 5 bands, real gap at the Cassini Division:

| Band | Inner (km) | Outer (km) | innerUnits | outerUnits | Shade |
| --- | ---: | ---: | ---: | ---: | --- |
| D ring | 66,900 | 74,510 | 1.149 | 1.280 | faint |
| C ring | 74,658 | 92,000 | 1.282 | 1.580 | medium |
| B ring | 92,000 | 117,580 | 1.580 | 2.019 | brightest, widest |
| *(Cassini Division — gap)* | 117,580 | 122,170 | 2.019 | 2.098 | — no band — |
| A ring | 122,170 | 136,775 | 2.098 | 2.349 | medium |
| F ring | 140,180 | 140,680 | 2.402 | 2.415 | thin, bright |

**Uranus** (radius 25,362 km) — 5 of its 13 named rings, all much narrower/darker (carbon-rich, not icy) than Saturn's:

| Band | innerUnits | outerUnits | Shade |
| --- | ---: | ---: | --- |
| 6/5/4 group | 1.660 | 1.668 | dark charcoal |
| alpha | 1.760 | 1.768 | dark charcoal |
| beta | 1.797 | 1.805 | dark charcoal |
| gamma/eta/delta group | 1.874 | 1.882 | dark charcoal |
| epsilon | 2.013 | 2.025 | widest, brightest of the set |

**Jupiter** (radius 69,911 km) — 3 bands, real rings but extremely faint dust, not ice like Saturn's:

| Band | innerUnits | outerUnits | Shade |
| --- | ---: | ---: | --- |
| Halo ring | 1.431 | 1.752 | very faint dark |
| Main ring | 1.752 | 1.845 | brightest of a faint set |
| Gossamer rings (combined) | 1.845 | 2.900 | very faint |

**Neptune** (radius 24,622 km) — 3 bands, also faint dust rings:

| Band | innerUnits | outerUnits | Shade |
| --- | ---: | ---: | --- |
| Galle ring | 1.661 | 1.742 | faint |
| Le Verrier + Lassell rings (combined) | 2.161 | 2.323 | faint |
| Adams ring | 2.550 | 2.562 | faint (real ring has bright "arcs" — see Limitations) |

Because a ring is a child of the planet's `CelestialBody`, it inherits that body's tilt+spin rotation the same way a moon would — but unlike a moon, this is **not** a limitation here: a full annulus is rotationally symmetric about its own axis, so spinning it about that same axis (the planet's Y-axis spin) produces zero visible change, frame to frame. Only the constant axial *tilt* matters, and that correctly orients the ring to the planet's equatorial plane, which is physically correct (real rings lie in their planet's equatorial plane).

## Material mapping

Flat color per band (see tables above) — no texture, no transparency (alpha blending isn't enabled in the renderer yet — that's shading-adjacent work, out of scope for this pass per current project direction: placement and behavior now, lighting/shading later).

## Limitations

- Uniform flat color per band, not continuously-varying real ring brightness/texture.
- Opaque, not the real rings' partial transparency.
- Only 5 bands each for Saturn/Uranus (not every one of Uranus's 13 named rings), 3 each for Jupiter/Neptune.
- Neptune's Adams ring is rendered as a uniform band; the real ring has distinct brighter "arcs" around part of its circumference, which would need per-vertex color (this project has none yet — flat per-`Material` color only) to represent.

## Verification

- `[SCENE] ring systems attached: Saturn (5 bands, Cassini Division gap), Uranus (5 bands), Jupiter (3 bands), Neptune (3 bands)` startup log line.
- Visual: fly to Saturn and confirm a visible gap between the B and A rings (Cassini Division); fly to Uranus and confirm five distinct thin dark bands rather than one solid disc; Jupiter and Neptune's bands are intentionally subtle (real rings), not missing.
