#version 330 core

in vec3 normal;
in vec2 texCoord;

out vec4 FragColor;

uniform vec3 baseColor;
uniform sampler2D albedoTexture;
uniform int useTexture;

void main()
{
	vec4 sampledColor = useTexture != 0
		? texture(albedoTexture, texCoord)
		: vec4(1.0);
	FragColor = vec4(sampledColor.rgb * baseColor, sampledColor.a);
}
