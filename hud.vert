#version 330 core
// Screen-space overlay (HUD and labels). Positions arrive in pixels with the
// origin at the top-left corner, exactly as TextRenderer lays them out.
layout (location = 0) in vec2 aPixel;
layout (location = 1) in vec4 aColor;

out vec4 color;

uniform vec2 screenSize;

void main()
{
    vec2 ndc = vec2(aPixel.x / screenSize.x * 2.0 - 1.0, 1.0 - aPixel.y / screenSize.y * 2.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
    color = aColor;
}
