#version 330 core
// Ray-traced view (F9). One primary ray per pixel against the analytic scene
// (spheres, ring annuli, the emissive Sun), shaded with the shared lighting
// model, a shadow ray to the Sun, rays that continue through translucent
// rings and one reflection bounce. See docs/objects/ray-tracing.md.

#include "lighting.glsl"
#include "raytrace.glsl"

in vec2 screenUv;
out vec4 FragColor;

uniform vec3 cameraForward;
uniform vec3 cameraRight;
uniform vec3 cameraUp;
uniform float tanHalfFov;
uniform float aspectRatio;
uniform float logDepthCoefficient;
uniform int maxBounces;
uniform int lightingEnabled;
uniform float ambientStrength;

uniform sampler2DArray albedoAtlas;
uniform mat3 sphereRotation[MAX_SPHERES]; // world -> body surface frame
uniform int sphereLayer[MAX_SPHERES];     // albedo layer in the atlas
uniform vec3 sphereMaterial[MAX_SPHERES]; // specular strength, power, reflectivity
uniform vec3 ringColors[MAX_RINGS];

const int MAX_LAYERS = 4;          // translucent ring layers one ray may cross
const float SURFACE_EPSILON = 1e-4;

// The same UV convention as UvSphereGenerator: u runs westward from the
// +X meridian (1 - azimuth / 2pi), v from the south pole (1 - polar / pi).
vec3 sphereAlbedo(int index, vec3 worldNormal)
{
	vec3 local = sphereRotation[index] * worldNormal;
	float azimuth = atan(local.z, local.x);
	if (azimuth < 0.0)
		azimuth += 2.0 * PI;
	vec2 uv = vec2(1.0 - azimuth / (2.0 * PI), 1.0 - acos(clamp(local.y, -1.0, 1.0)) / PI);
	// Screen-space gradients with the longitude wrap removed, so the u = 0/1
	// seam does not select the smallest mip level and draw a blurred line.
	vec2 dx = dFdx(uv);
	vec2 dy = dFdy(uv);
	dx.x -= round(dx.x);
	dy.x -= round(dy.x);
	return textureGrad(albedoAtlas, vec3(uv, float(sphereLayer[index])), dx, dy).rgb;
}

vec3 lightSurface(vec3 albedo, vec3 p, vec3 n, vec3 v, bool twoSided, float specularStrength, float specularPower)
{
	if (lightingEnabled == 0)
		return albedo;
	LightTerms terms = evaluateLights(n, v, p, twoSided, SHADING_BLINN_PHONG, specularStrength, specularPower);
	float sunlight = sunVisibility(p);  // the shadow ray
	return albedo * (ambientStrength + terms.sunDiffuse * sunlight + terms.otherDiffuse) +
		terms.sunSpecular * sunlight + terms.otherSpecular;
}

// Follows one ray through up to MAX_LAYERS translucent ring bands until it
// meets a sphere or leaves the scene. Returns the colour gathered, the
// transmittance left, the first hit distance, and the opaque sphere hit.
struct RayResult
{
	vec3 color;
	float transmittance;
	float firstT;        // distance to the first thing hit, -1 on a miss
	int sphere;          // opaque sphere index, -1 if none
	vec3 point;
	vec3 normal;
	float opaqueWeight;  // transmittance in front of the opaque hit
};

RayResult castRay(vec3 origin, vec3 direction)
{
	RayResult result;
	result.color = vec3(0.0);
	result.transmittance = 1.0;
	result.firstT = -1.0;
	result.sphere = -1;
	result.point = vec3(0.0);
	result.normal = vec3(0.0);
	result.opaqueWeight = 0.0;

	vec3 rayOrigin = origin;
	float travelled = 0.0;
	for (int layer = 0; layer < MAX_LAYERS; ++layer)
	{
		// Nearest sphere.
		float nearest = 1e30;
		int hitSphere = -1;
		for (int i = 0; i < MAX_SPHERES; ++i)
		{
			if (i >= sphereCount)
				break;
			float t = intersectSphere(rayOrigin, direction, spheres[i], SURFACE_EPSILON);
			if (t > 0.0 && t < nearest)
			{
				nearest = t;
				hitSphere = i;
			}
		}
		// Nearest ring band in front of that sphere.
		int hitRing = -1;
		for (int i = 0; i < MAX_RINGS; ++i)
		{
			if (i >= ringCount)
				break;
			float t = intersectRing(rayOrigin, direction, i, SURFACE_EPSILON);
			if (t > 0.0 && t < nearest)
			{
				nearest = t;
				hitRing = i;
				hitSphere = -1;
			}
		}
		if (hitSphere < 0 && hitRing < 0)
			break;

		vec3 p = rayOrigin + direction * nearest;
		if (result.firstT < 0.0)
			result.firstT = travelled + nearest;

		if (hitRing >= 0)
		{
			// Thin two-sided sheet: shade it, keep going behind it.
			vec3 n = ringNormals[hitRing].xyz;
			if (dot(n, direction) > 0.0)
				n = -n;
			vec3 shaded = lightSurface(ringColors[hitRing], p, n, -direction, true, 0.0, 8.0);
			float opacity = ringOpacity[hitRing];
			result.color += result.transmittance * opacity * shaded;
			result.transmittance *= 1.0 - opacity;
			travelled += nearest;
			rayOrigin = p;
			continue;
		}

		// Opaque sphere: the ray ends here.
		vec3 n = normalize(p - spheres[hitSphere].xyz);
		vec3 albedo = sphereAlbedo(hitSphere, n);
		vec3 shaded = sphereEmissive[hitSphere] != 0 ? albedo * 1.25
			: lightSurface(albedo, p, n, -direction, false, sphereMaterial[hitSphere].x, sphereMaterial[hitSphere].y);
		result.opaqueWeight = result.transmittance;
		result.color += result.transmittance * shaded;
		result.transmittance = 0.0;
		result.sphere = hitSphere;
		result.point = p;
		result.normal = n;
		break;
	}
	return result;
}

void main()
{
	// Primary ray through this pixel (pinhole camera, same field of view
	// and basis as the raster projection).
	vec2 ndc = screenUv * 2.0 - 1.0;
	vec3 direction = normalize(cameraForward + ndc.x * tanHalfFov * aspectRatio * cameraRight +
		ndc.y * tanHalfFov * cameraUp);

	RayResult primary = castRay(vec3(0.0), direction);

	// Whitted reflection: one mirror bounce off reflective (icy, ocean) worlds.
	if (primary.sphere >= 0 && maxBounces > 0 && sphereEmissive[primary.sphere] == 0)
	{
		float reflectivity = sphereMaterial[primary.sphere].z;
		if (reflectivity > 0.0)
		{
			vec3 bounceDirection = reflect(direction, primary.normal);
			RayResult bounce = castRay(primary.point + primary.normal * SURFACE_EPSILON * 10.0, bounceDirection);
			primary.color += primary.opaqueWeight * reflectivity * bounce.color;
		}
	}

	// Solar glow: how close the primary ray passes to the Sun's centre.
	vec3 glow = vec3(0.0);
	float glowAlpha = 0.0;
	float along = dot(sunCenter, direction);
	if (along > 0.0 && (primary.firstT < 0.0 || primary.firstT > along))
	{
		float missDistance = length(sunCenter - direction * along);
		float sunRadius = 0.0;
		for (int i = 0; i < MAX_SPHERES; ++i)
		{
			if (i < sphereCount && sphereEmissive[i] != 0)
				sunRadius = spheres[i].w;
		}
		if (sunRadius > 0.0 && missDistance > sunRadius)
		{
			glowAlpha = 0.85 * exp(-(missDistance - sunRadius) / (sunRadius * 0.45));
			glow = vec3(1.0, 0.72, 0.36);
		}
	}

	if (primary.firstT < 0.0)
	{
		if (glowAlpha < 0.004)
			discard;
		// Glow over empty sky: blend at the far plane, behind everything.
		FragColor = vec4(glow, glowAlpha);
		gl_FragDepth = 1.0;
		return;
	}

	float coverage = 1.0 - primary.transmittance;
	vec3 color = primary.color / max(coverage, 1e-4);
	FragColor = vec4(color + glow * glowAlpha, coverage);
	float viewDepth = primary.firstT * dot(direction, cameraForward);
	gl_FragDepth = log2(1.0 + viewDepth) * logDepthCoefficient * 0.5;
}
