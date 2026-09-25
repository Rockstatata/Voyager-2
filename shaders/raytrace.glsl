// Analytic ray tracing against the solar system's exact primitives: every
// body is a sphere, every ring band an annulus (docs/objects/ray-tracing.md).
// Used by scene.frag for ray-traced shadows and by raytrace.frag for the
// fully ray-traced view. All positions are camera-relative.

#include "raytrace_mesh.glsl"

#define MAX_SPHERES 32
#define MAX_RINGS 16
#define PI 3.14159265358979

uniform int sphereCount;
uniform vec4 spheres[MAX_SPHERES];         // xyz centre, w radius
uniform int sphereEmissive[MAX_SPHERES];   // 1 for the Sun (never an occluder)
uniform int ringCount;
uniform vec4 ringCenters[MAX_RINGS];       // xyz centre, w inner radius
uniform vec4 ringNormals[MAX_RINGS];       // xyz unit normal, w outer radius
uniform float ringOpacity[MAX_RINGS];
uniform vec3 sunCenter;                    // light 0 position
uniform float sunLightRadius;              // radius of the Sun as an area light
uniform int shadowMode;                    // 0 off, 1 hard (point light), 2 soft (area light)

// Ray-sphere: solve |o + t d - c|^2 = r^2 for the nearest t > tMin.
// With d unit length the quadratic is t^2 + 2 b t + c = 0, b = dot(o - c, d).
// Returns -1 on a miss.
float intersectSphere(vec3 origin, vec3 direction, vec4 sphere, float tMin)
{
	vec3 oc = origin - sphere.xyz;
	float b = dot(oc, direction);
	float c = dot(oc, oc) - sphere.w * sphere.w;
	float discriminant = b * b - c;
	if (discriminant < 0.0)
		return -1.0;
	float root = sqrt(discriminant);
	float t = -b - root;          // entering the sphere
	if (t > tMin)
		return t;
	t = -b + root;                // origin inside: the exit point
	return t > tMin ? t : -1.0;
}

// Ray-annulus: intersect the ring's plane, then keep the hit if its distance
// from the centre lies between the inner and outer radius.
float intersectRing(vec3 origin, vec3 direction, int ring, float tMin)
{
	vec3 normal = ringNormals[ring].xyz;
	float denominator = dot(direction, normal);
	if (abs(denominator) < 1e-7)
		return -1.0;              // ray parallel to the ring plane
	float t = dot(ringCenters[ring].xyz - origin, normal) / denominator;
	if (t <= tMin)
		return -1.0;
	float radius = length(origin + direction * t - ringCenters[ring].xyz);
	return (radius >= ringCenters[ring].w && radius <= ringNormals[ring].w) ? t : -1.0;
}

// Fraction of a disc of angular radius r1 covered by a disc of angular
// radius r2 whose centres are `separation` apart (circle-circle overlap).
// This is how much of the Sun an occluding body hides: an analytic
// area-light shadow with a real umbra and penumbra.
float discCoverage(float r1, float r2, float separation)
{
	if (separation >= r1 + r2)
		return 0.0;
	if (separation <= abs(r2 - r1))
		return r2 >= r1 ? 1.0 : (r2 * r2) / (r1 * r1);
	float a = r1 * r1 * acos(clamp((separation * separation + r1 * r1 - r2 * r2) / (2.0 * separation * r1), -1.0, 1.0));
	float b = r2 * r2 * acos(clamp((separation * separation + r2 * r2 - r1 * r1) / (2.0 * separation * r2), -1.0, 1.0));
	float c = 0.5 * sqrt(max((-separation + r1 + r2) * (separation + r1 - r2) * (separation - r1 + r2) * (separation + r1 + r2), 0.0));
	return clamp((a + b - c) / (PI * r1 * r1), 0.0, 1.0);
}

// Shadow ray from surface point p toward the Sun. Returns the fraction of
// sunlight that arrives (1 = fully lit, 0 = umbra). With testMesh the ray is
// also traced through Voyager's BVH, so its dish can shade its own bus.
float sunVisibility(vec3 p, bool testMesh)
{
	if (shadowMode == 0)
		return 1.0;

	vec3 toSun = sunCenter - p;
	float sunDistance = length(toSun);
	vec3 direction = toSun / sunDistance;
	float sunAngle = asin(clamp(sunLightRadius / sunDistance, 0.0, 1.0));
	float visibility = 1.0;

	for (int i = 0; i < MAX_SPHERES; ++i)
	{
		if (i >= sphereCount)
			break;
		if (sphereEmissive[i] != 0)
			continue;
		vec3 toCentre = spheres[i].xyz - p;
		float centreDistance = length(toCentre);
		float radius = spheres[i].w;
		// Skip the sphere this point lies on (or inside): no self-shadowing,
		// the Lambert term already darkens its own night side.
		if (centreDistance <= radius * 1.001)
			continue;
		float along = dot(toCentre, direction);
		if (along <= 0.0 || along >= sunDistance)
			continue;             // behind the point or beyond the Sun

		if (shadowMode == 1)
		{
			// Hard shadow: does the single ray to the Sun's centre hit it?
			if (intersectSphere(p, direction, spheres[i], 0.0) > 0.0)
				visibility = 0.0;
		}
		else
		{
			// Soft shadow: compare the angular discs of Sun and occluder.
			float occluderAngle = asin(clamp(radius / centreDistance, 0.0, 1.0));
			float separation = acos(clamp(dot(toCentre / centreDistance, direction), -1.0, 1.0));
			visibility *= 1.0 - discCoverage(sunAngle, occluderAngle, separation);
		}
	}

	if (testMesh && visibility > 0.0)
	{
		// Any triangle between the point and the Sun blocks it completely
		// (the spacecraft is tiny next to the Sun's disc: a hard shadow).
		if (intersectMesh(p, direction, 0.0, sunDistance, true).t > 0.0)
			return 0.0;
	}

	for (int i = 0; i < MAX_RINGS; ++i)
	{
		if (i >= ringCount)
			break;
		// A ring point must not shadow itself: start the ray a little off
		// its own plane.
		float tMin = ringNormals[i].w * 1e-4;
		float t = intersectRing(p, direction, i, tMin);
		if (t > 0.0 && t < sunDistance)
			visibility *= 1.0 - ringOpacity[i];
	}
	return visibility;
}
