#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
// A per-instance model matrix, split into four vec4 columns because GLSL/GL
// vertex attributes cap out at vec4 — a mat4 attribute is really four
// consecutive locations under the hood. Only bound/advanced-per-instance
// (glVertexAttribDivisor) when Mesh::drawInstanced is used; see Mesh.cpp.
layout (location = 3) in vec4 aInstanceModelCol0;
layout (location = 4) in vec4 aInstanceModelCol1;
layout (location = 5) in vec4 aInstanceModelCol2;
layout (location = 6) in vec4 aInstanceModelCol3;

out vec3 normal;
out vec2 texCoord;
out vec3 relativePosition; // camera-relative world position (floating origin)
out float logDepthW;

// `model` is already camera-relative: Renderer subtracts the camera position
// in double precision before narrowing (bible section 20). For instanced
// draws it holds only that origin shift and multiplies each world instance.
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform int useInstancing;

void main()
{
    mat4 effectiveModel = useInstancing != 0
        ? model * mat4(aInstanceModelCol0, aInstanceModelCol1, aInstanceModelCol2, aInstanceModelCol3)
        : model;

    vec4 relative = effectiveModel * vec4(aPos, 1.0);
    relativePosition = relative.xyz;
    gl_Position = proj * view * relative;
    logDepthW = 1.0 + gl_Position.w;
    gl_PointSize = 2.0;
    normal = mat3(transpose(inverse(effectiveModel))) * aNormal;
    texCoord = aTexCoord;
}
