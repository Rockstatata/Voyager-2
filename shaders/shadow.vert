#version 330 core
// Depth-only pass for the shadow map (src/rendering/ShadowMap.cpp): the
// spacecraft drawn from the Sun. Only the position attribute is read.
layout (location = 0) in vec3 aPos;

uniform mat4 model;       // camera-relative
uniform mat4 lightMatrix; // orthographic projection * view from the Sun

void main()
{
    gl_Position = lightMatrix * model * vec4(aPos, 1.0);
}
