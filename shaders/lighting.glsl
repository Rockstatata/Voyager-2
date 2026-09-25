// Shared lighting model, included by scene.vert (Gouraud: evaluated per
// vertex) and scene.frag (Flat, Phong, Blinn-Phong, Toon: per fragment).
// Every position and direction is camera-relative (floating origin).

#define MAX_LIGHTS 4
#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT 1
#define LIGHT_SPOT 2

#define SHADING_FLAT 0
#define SHADING_GOURAUD 1
#define SHADING_PHONG 2
#define SHADING_BLINN_PHONG 3
#define SHADING_TOON 4

struct Light
{
	int type;
	int enabled;
	vec3 position;
	vec3 direction;     // the way the light travels
	vec3 color;         // already multiplied by intensity
	vec3 attenuation;   // constant, linear, quadratic
	float innerCutoff;  // cosines
	float outerCutoff;
};

uniform Light lights[MAX_LIGHTS];
uniform int shadingTechnique;
uniform float specularStrength;
uniform float specularPower;

// Diffuse and specular contributions, kept apart for the Sun (light 0, the
// only shadow caster) and every other light.
struct LightTerms
{
	vec3 sunDiffuse;
	vec3 sunSpecular;
	vec3 otherDiffuse;
	vec3 otherSpecular;
};

// Toon diffuse: four flat bands instead of a smooth ramp.
float toonBand(float lambert)
{
	if (lambert > 0.75) return 1.0;
	if (lambert > 0.40) return 0.62;
	if (lambert > 0.12) return 0.30;
	return 0.06;
}

// n: unit normal facing the viewer's side for two-sided sheets.
// v: unit vector to the eye.  p: camera-relative surface position.
LightTerms evaluateLights(vec3 n, vec3 v, vec3 p, bool twoSided, int technique)
{
	LightTerms terms;
	terms.sunDiffuse = vec3(0.0);
	terms.sunSpecular = vec3(0.0);
	terms.otherDiffuse = vec3(0.0);
	terms.otherSpecular = vec3(0.0);

	for (int i = 0; i < MAX_LIGHTS; ++i)
	{
		if (lights[i].enabled == 0)
			continue;

		// Direction to the light and distance attenuation.
		vec3 l;
		float attenuation = 1.0;
		if (lights[i].type == LIGHT_DIRECTIONAL)
		{
			l = normalize(-lights[i].direction);
		}
		else
		{
			vec3 toLight = lights[i].position - p;
			float distance = length(toLight);
			l = toLight / max(distance, 1e-9);
			vec3 k = lights[i].attenuation;
			attenuation = 1.0 / max(k.x + k.y * distance + k.z * distance * distance, 1e-4);
			if (lights[i].type == LIGHT_SPOT)
			{
				// Smooth edge between the inner and outer cone.
				float theta = dot(-l, normalize(lights[i].direction));
				float edge = max(lights[i].innerCutoff - lights[i].outerCutoff, 1e-4);
				attenuation *= clamp((theta - lights[i].outerCutoff) / edge, 0.0, 1.0);
			}
		}
		if (attenuation <= 0.0)
			continue;

		float lambert = dot(n, l);
		if (twoSided)
			lambert = abs(lambert);
		float diffuse = max(lambert, 0.0);
		if (technique == SHADING_TOON)
			diffuse = lambert > 0.0 ? toonBand(diffuse) : 0.0;

		float specular = 0.0;
		if (lambert > 0.0 && specularStrength > 0.0)
		{
			if (technique == SHADING_PHONG || technique == SHADING_GOURAUD)
			{
				// Phong: angle between the mirror reflection and the eye.
				vec3 r = reflect(-l, n);
				specular = pow(max(dot(v, r), 0.0), specularPower);
			}
			else
			{
				// Blinn-Phong: angle between the normal and the half vector.
				// The exponent is doubled so highlight size matches Phong.
				vec3 h = normalize(l + v);
				specular = pow(max(dot(n, h), 0.0), specularPower * 2.0);
				if (technique == SHADING_TOON)
					specular = step(0.5, specular);
			}
			specular *= specularStrength;
		}

		vec3 radiance = lights[i].color * attenuation;
		if (i == 0)
		{
			terms.sunDiffuse += diffuse * radiance;
			terms.sunSpecular += specular * radiance;
		}
		else
		{
			terms.otherDiffuse += diffuse * radiance;
			terms.otherSpecular += specular * radiance;
		}
	}
	return terms;
}
