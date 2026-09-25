# HUD, body labels and help overlay

![Controls overlay over a Uranus Focus view](images/runtime/12_help.jpg)

*Runtime capture: the `F1` controls overlay over a Uranus Focus view, with the HUD panel top left, depth-sorted body labels and the encounter banner at the bottom.*

Bible sections 39 and 40 and Phase 12 ("labels") ask for telemetry, selected-object feedback and in-scene names. Everything on this page is drawn by `TextRenderer` (`src/rendering/TextRenderer.*`) using the project-authored 5x7 font in `src/rendering/BitmapFont.cpp` and the `hud.vert`/`hud.frag` shaders. No font file, glyph atlas or text library is used.

## Geometry generation

- **Font.** Each glyph is seven rows of five characters written as text in `BitmapFont.cpp`, for example `'A': ".###.", "#...#", "#...#", "#####", ...`. At first use they are packed into 7-byte bitmasks (bit 4 is the leftmost cell). Lower-case letters map to upper case. An unknown character draws a hollow box, so a missing glyph is visible instead of silent.
- **Quads.** `TextRenderer::addText` walks the string and emits one screen-space quad (two triangles, six vertices) for every lit cell. The pen advances six cells per character and nine cells per line. `addRect` emits a quad for panels and label ticks, and `addShadowedText` draws the string twice with a one-cell black offset for readability.
- **One draw per frame.** All vertices for the frame collect in a CPU vector. `flush` refills a single `GL_DYNAMIC_DRAW` buffer, orphaning it with `glBufferData`, and issues one `glDrawArrays(GL_TRIANGLES)`. The VAO and VBO are created once in `initialize`, which avoids bible F8.

## Vertex attributes

The overlay does not use the 3D `Vertex` format. It has its own six-float layout:

| Location | Attribute | Components |
| --- | --- | --- |
| 0 | pixel position (origin top-left) | 2 |
| 1 | RGBA colour | 4 |

`hud.vert` maps pixels to NDC with `x = px / width * 2 - 1` and `y = 1 - py / height * 2`. The overlay is drawn after the scene with depth test and culling off and alpha blending on, then the renderer's state is restored.

## Scale

Glyph cell size is `round(framebuffer height / 450)` pixels, which is 2 px at 900 lines and 4 px at 4K. The text stays readable at any window size, and the window can be resized freely.

## HUD panel

The top-left panel shows:

- the title;
- the UTC date and Julian Date of the shared simulation clock ([mission-ephemeris.md](mission-ephemeris.md));
- the time rate in days or hours per second, the speed multiplier, and whether encounter slow-motion is active;
- the camera mode and free-flight wheel speed;
- the flight mode;
- Voyager's real heliocentric distance in AU and speed in km/s, and the nearest planet with its distance in km. All three come from the Horizons state vectors, not from render units;
- the radius of the focused body.

In Manual flight the panel shows ship speed in render units per second instead of the historical telemetry. Within ±72 hours of a closest approach a banner at the bottom counts down (`T-06:00`) and then up, and states the real closest-approach distance.

## Labels

`HudOverlay::renderLabels` projects each body centre with the same view and projection as the scene. A name is placed above the projected disc, with a small tick when the body is under two pixels across. A label is skipped when:

- the body is behind the camera or off screen;
- the disc fills more than 45 % of the screen height, so the name would cover the surface;
- a nearer sphere covers the body's centre (ray-sphere test against all 26 bodies), so names never show through planets;
- it would overlap a label already placed, or the HUD panel. Placement order is Sun and planets nearest first, then Voyager, then moons.

A moon is named only when the camera is within 10 parent radii of its planet, so a system's labels appear as the camera arrives and do not clutter the overview. `L` toggles all labels and `F2` toggles the HUD. `F1` shows the controls overlay, which lists every key; that list is in [controls.md](controls.md).

## Lighting and render status

Below the telemetry, `LightingController::statusLines` adds the current shading technique and shadow mode (`SHADING BLINN-PHONG (F3)   RAY-TRACED SHADOWS SOFT (F4)`) and the light rig (`LIGHTS  SUN  HEADLAMP OFF  FILL OFF  MAPS ON`), followed by `RENDER RASTER (F9 RAY-TRACE)` or `RENDER RAY-TRACED (F9)  REFLECTIONS ON (F10)`. These arrive through `HudView::extraLines`, so the HUD never needs to know about lighting types.

## Inspect captions and component labels

In Inspect mode (`I`) the panel's camera line reads `INSPECT VOYAGER  (, . COMPONENT  I EXIT)`. A caption at the bottom names the current component with its position in the cycle (`HIGH-GAIN ANTENNA  (2/18)`) and a one-line fact (`HudView::caption`, `captionDetail`). Component centres are projected like body centres. The whole-craft view labels all 18 components; a close-up labels only the inspected one, in the accent colour, so labels never bury the hardware.

## Hint bar and notices

A single line along the bottom (`HudView::hints`, built by `Application::keyHints`) lists the keys that matter in the current mode: free flight, Focus, Inspect, Manual piloting or the default Chase. `HudView::notice` shows short warnings, such as `PRESS ESC AGAIN TO QUIT` for the two seconds after the first `Esc`.

## Limitations

- The font has only upper-case glyphs and a fixed 5x7 pixel grid, with no anti-aliasing or kerning.
- Label overlap is resolved greedily: a lower-priority name is hidden rather than moved.

## Verification

1. At startup the HUD shows `1977-08-21 00:00 UTC   JD 2443376.50`, and `NEAREST EARTH` is about 323,000 km.
2. Press `H`. The overview labels the Sun and the planets, and Mercury's label may be hidden where it would collide with the Sun's.
3. Press `F1`, and the overlay matches the capture above. Resize the window, and the text keeps its proportions.
4. Press `I`, then `.`. The caption changes to the next component and only its label is shown. Press `F3`: the shading line in the panel changes.
5. Press `2`. The banner appears, counts through `T-00:00`, and shows `CLOSEST APPROACH 721,351 KM FROM CENTRE`.
