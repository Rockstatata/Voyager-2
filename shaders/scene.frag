#version 330 core

#include "lighting.glsl"

in vec3 normal;
in vec2 texCoord;
in vec3 relativePosition;
in float logDepthW;
in vec3 gouraudSunDiffuse;
in vec3 gouraudSunSpecular;
in vec3 gouraudOtherDiffuse;
in vec3 gouraudOtherSpecular;

out vec4 FragColor;

uniform vec3 baseColor;
uniform sampler2D albedoTexture;
uniform int useTexture;

// Material shading model: 0 unlit, 1 lit, 2 lit two-sided, 3 additive glow.
uniform int shadingModel;
uniform float opacity;
uniform int lightingEnabled;
uniform float ambientStrength;

// Logarithmic depth: 2 / log2(far + 1). Keeps centimetre and thousand-unit
// geometry in one depth buffer without z-fighting.
uniform float logDepthCoefficient;

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

	bool twoSided = shadingModel == 2;
	if (shadingTechnique == SHADING_FLAT)
	{
		// One normal per triangle: the plane through the screen-space
		// derivatives of the position, oriented like the interpolated normal.
		vec3 faceNormal = normalize(cross(dFdx(relativePosition), dFdy(relativePosition)));
		surfaceNormal = dot(faceNormal, surfaceNormal) < 0.0 ? -faceNormal : faceNormal;
	}
	if (twoSided && dot(surfaceNormal, viewDirection) < 0.0)
		surfaceNormal = -surfaceNormal;

	LightTerms terms;
	if (shadingTechnique == SHADING_GOURAUD)
	{
		terms.sunDiffuse = gouraudSunDiffuse;
		terms.sunSpecular = gouraudSunSpecular;
		terms.otherDiffuse = gouraudOtherDiffuse;
		terms.otherSpecular = gouraudOtherSpecular;
	}
	else
	{
		terms = evaluateLights(surfaceNormal, viewDirection, relativePosition, twoSided, shadingTechnique);
	}

	vec3 diffuse = terms.sunDiffuse + terms.otherDiffuse;
	vec3 specular = terms.sunSpecular + terms.otherSpecular;
	vec3 lit = color * (ambientStrength + diffuse) + specular;

	if (shadingTechnique == SHADING_TOON)
	{
		// Ink outline: darken where the surface turns away from the eye.
		float rim = dot(surfaceNormal, viewDirection);
		if (rim < 0.22)
			lit *= 0.15;
	}

	FragColor = vec4(lit, albedo.a * opacity);
}
