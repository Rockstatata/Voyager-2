# Termination shock and heliopause

![Voyager 2 at the heliopause crossing](images/runtime/07_heliopause.jpg)

*Runtime capture, bookmark `6`: Voyager 2 at 119.0 AU on 2018-11-05, the day it crossed the heliopause.*

The three great-circle meshes use the line-circle construction in the [Procedural Mesh Construction Handbook](procedural-meshes.md).

## Geometry

Two wireframe spheres centred on the Sun: the termination shock and the heliopause. Each is the shared unit `CircleGenerator` loop (96 vertices, `GL_LINE_LOOP`) drawn three times, rotated into the XZ, XY and YZ planes. Three great circles read as a sphere from any angle, without a solid shell hiding what is inside. Two boundaries cost six small draws and one shared upload.

## Transform and units

The radii are the distances where Voyager 2 itself crossed each boundary: the termination shock at 84 AU (2007-08-30) and the heliopause at 119 AU (2018-11-05). `ScaleManager::distanceAuToRenderUnits` maps them to 231.3 and 280.2 units, the same law used for Voyager's path ([scale-manager.md](scale-manager.md)). So the drawn path crosses each circle where the spacecraft did. Each loop is translated to the Sun and uniformly scaled.

## Material

`Unlit` flat colour: dim blue for the termination shock and dim violet for the heliopause. The colours are deliberately faint so the circles guide the eye without cluttering the view.

## Limitations

- The real heliosphere is asymmetric and changes with the solar cycle. A sphere at Voyager 2's crossing distances is a simplification.
- The heliosheath has no volume.

## Verification

1. Press `T`, then fly out with `Shift` and the wheel. The gold path crosses the blue circle and then the violet one.
2. Press `6`. The HUD reads `VOYAGER 119.02 AU FROM SUN` on 2018-11-05.
