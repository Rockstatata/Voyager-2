#version 330 core

#include "lighting.glsl"
#include "raytrace.glsl"

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

// Lighting maps (units 1 and 2) and their master switch (F8).
uniform sampler2D normalMap;
uniform sampler2D specularMap;
uniform int useNormalMap;
uniform int useSpecularMap;
uniform float normalStrength;
uniform int surfaceMapsEnabled;

// Material shading model: 0 unlit, 1 lit, 2 lit two-sided, 3 additive glow.
uniform int shadingModel;
uniform float opacity;
uniform int lightingEnabled;
uniform float ambientStrength;

// Logarithmic depth: 2 / log2(far + 1). Keeps centimetre and thousand-unit
// geometry in one depth buffer without z-fighting.
uniform float logDepthCoefficient;

// Normal mapping without stored tangents: the tangent frame is rebuilt per
// pixel from screen-space derivatives of position and UV (Schueler, 2006),
// so the shared sphere's vertex format stays position/normal/UV.
vec3 perturbNormal(vec3 n, vec3 p, vec2 uv)
{
	vec3 dp1 = dFdx(p);
	vec3 dp2 = dFdy(p);
	vec2 duv1 = dFdx(uv);
	vec2 duv2 = dFdy(uv);
	vec3 dp2perp = cross(dp2, n);
	vec3 dp1perp = cross(n, dp1);
	vec3 tangent = dp2perp * duv1.x + dp1perp * duv2.x;
	vec3 bitangent = dp2perp * duv1.y + dp1perp * duv2.y;
	float scale = inversesqrt(max(max(dot(tangent, tangent), dot(bitangent, bitangent)), 1e-20));
	mat3 tbn = mat3(tangent * scale, bitangent * scale, n);
	vec3 mapped = texture(normalMap, uv).xyz * 2.0 - 1.0;
	mapped.xy *= normalStrength;
	return normalize(tbn * mapped);
}

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

	bool mapsOn = surfaceMapsEnabled != 0 && shadingTechnique != SHADING_GOURAUD;
	if (mapsOn && useNormalMap != 0 && shadingTechnique != SHADING_FLAT)
		surfaceNormal = perturbNormal(surfaceNormal, relativePosition, texCoord);
	float specularScale = (mapsOn && useSpecularMap != 0) ? texture(specularMap, texCoord).r : 1.0;

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
		terms = evaluateLights(surfaceNormal, viewDirection, relativePosition, twoSided, shadingTechnique, specularScale);
	}

	// Ray-traced shadow: one shadow ray per pixel toward the Sun, tested
	// against every body sphere and ring band (raytrace.glsl).
	float sunlight = sunVisibility(relativePosition);
	vec3 diffuse = terms.sunDiffuse * sunlight + terms.otherDiffuse;
	vec3 specular = terms.sunSpecular * sunlight + terms.otherSpecular;
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
