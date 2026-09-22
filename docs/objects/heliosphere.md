# Termination shock and heliopause

The three great-circle meshes use the exact line-circle construction documented in the [Procedural Mesh Construction Handbook](procedural-meshes.md).

## Geometry

The outer heliosphere is represented by two wireframe spherical boundaries centered on the Sun: the termination shock at 94 AU and the heliopause at 119 AU. Each boundary reuses the orbit guide's unit `CircleGenerator` mesh three times, rotated into the XZ, XY, and YZ planes. Those three great circles read as a 3D sphere from any camera angle without requiring transparency or a solid shell that would hide planets and stars.

Each unit circle has 96 vertices/indices and is drawn with `GL_LINE_LOOP`; the closing edge is supplied by OpenGL. Two boundaries therefore cost six small draw calls but introduce no new geometry upload.

## Transform and units

The real AU radii are converted to kilometres (`1 AU = 149,597,870.7 km`) and then passed through `ScaleManager::distanceToRenderUnits`, using the same logarithmic educational distance mapping as planets and the Voyager trajectory. Each loop is translated to the Sun and uniformly scaled to the mapped boundary radius.

The quoted 94/119 AU values are representative crossing distances, not claims that the heliosphere is a perfect static sphere; the real boundary varies with solar activity and direction.

## Material and limitation

The termination shock is muted blue and the heliopause muted violet. These are flat guide colors, not physical emissions. Lighting, transparency, plasma flow, heliosheath thickness, and a directional/asymmetric heliosphere are deferred.

## Verification

- Startup reports `termination-shock/heliopause wireframes`.
- In FreeFly, moving outside the planetary region reveals two nested three-axis wire spheres before the Oort field.
- The loops remain centered on the Sun and do not occlude objects inside them.
