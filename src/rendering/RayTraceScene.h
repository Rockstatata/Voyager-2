#ifndef RAY_TRACE_SCENE_H
#define RAY_TRACE_SCENE_H

#include <vector>

#include <glm/glm.hpp>

// The analytic primitives the ray tracer intersects (docs/objects/ray-tracing.md).
// Every celestial body is an exact sphere and every ring band an exact
// annulus, so rays hit them with closed-form equations instead of searching
// thousands of triangles. Rebuilt every frame from the scene (world space,
// double precision); the Renderer uploads it camera-relative.
struct TraceSphere
{
	glm::dvec3 center{ 0.0 };
	double radius = 0.0;
	// World-to-surface rotation (inverse tilt and spin) and the body's
	// albedo layer, so the full ray-traced view can texture its hits.
	glm::mat3 worldToLocal{ 1.0f };
	int textureLayer = -1;
	float specularStrength = 0.0f;
	float specularPower = 16.0f;
	float reflectivity = 0.0f;
	bool emissive = false;   // the Sun: never an occluder, seen as a light
};

struct TraceRing
{
	glm::dvec3 center{ 0.0 };
	glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
	double innerRadius = 0.0;   // world units
	double outerRadius = 0.0;
	glm::vec3 color{ 1.0f };
	float opacity = 1.0f;       // fraction of light the band blocks
};

struct RayTraceScene
{
	std::vector<TraceSphere> spheres;
	std::vector<TraceRing> rings;
	glm::dvec3 sunPosition{ 0.0 };
	// Radius of the Sun as a LIGHT (sets penumbra width). Smaller than its
	// display radius so eclipse shadows keep a visible umbra (lighting.md).
	double sunLightRadius = 0.6;

	static constexpr int kMaxSpheres = 32;
	static constexpr int kMaxRings = 16;
};

#endif
