#version 330 core
// One triangle that covers the whole screen, generated from gl_VertexID
// (no vertex buffer): (-1,-1), (3,-1), (-1,3). Every pixel inside the
// viewport runs raytrace.frag exactly once.
out vec2 screenUv; // 0..1 across the viewport

void main()
{
    vec2 corner = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    screenUv = corner;
    gl_Position = vec4(corner * 2.0 - 1.0, 0.0, 1.0);
}
