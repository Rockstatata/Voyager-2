#version 330 core

in vec3 normal;
in vec2 texCoord;
in vec3 relativePosition;
in float logDepthW;

out vec4 FragColor;

uniform vec3 baseColor;
uniform sampler2D albedoTexture;
uniform int useTexture;

// Material shading model: 0 unlit, 1 lit, 2 lit two-sided, 3 additive glow.
uniform int shadingModel;
uniform float specularStrength;
uniform float specularPower;
uniform float opacity;

// The Sun is the only light: a point source at its centre (camera-relative).
uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform int lightingEnabled;

// Logarithmic depth: 2 / log2(far + 1). Keeps centimetre and thousand-unit
// geometry in one depth buffer without z-fighting.
uniform float logDepthCoefficient;

const float kAmbient = 0.07;

void main()
{
	gl_FragDepth = log2(logDepthW) * logDepthCoefficient * 0.5;

	vec4 albedo = useTexture != 0 ? texture(albedoTexture, texCoord) : vec4(1.0);
	vec3 color = albedo.rgb * baseColor;
	vec3 viewDirection = normalize(-relativePosition);
	vec3 surfaceNormal = normalize(normal);

	if (shadingModel == 3)
	{
		// Soft halo shell: opaque-looking toward the centre of the shell,
		// fading to nothing at its silhouette. specularPower is the falloff.
		float facing = abs(dot(surfaceNormal, viewDirection));
		FragColor = vec4(color, pow(facing, specularPower) * opacity);
		return;
	}

	if (shadingModel == 0 || lightingEnabled == 0)
	{
		FragColor = vec4(color, albedo.a * opacity);
		return;
	}

	vec3 toLight = normalize(lightPosition - relativePosition);
	float lambert = dot(surfaceNormal, toLight);
	if (shadingModel == 2)
	{
		lambert = abs(lambert);
		if (dot(surfaceNormal, viewDirection) < 0.0)
			surfaceNormal = -surfaceNormal;
	}
	float diffuse = max(lambert, 0.0);

	// Blinn-Phong highlight only on the lit side.
	float specular = 0.0;
	if (diffuse > 0.0 && specularStrength > 0.0)
	{
		vec3 halfway = normalize(toLight + viewDirection);
		specular = specularStrength * pow(max(dot(surfaceNormal, halfway), 0.0), specularPower);
	}

	vec3 lit = color * (kAmbient + diffuse * lightColor) + specular * lightColor;
	FragColor = vec4(lit, albedo.a * opacity);
}
