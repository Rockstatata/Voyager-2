#ifndef RAY_TRACER_H
#define RAY_TRACER_H

#include <string>
#include <vector>

#include <glad/glad.h>

#include "Lighting.h"
#include "RayTraceScene.h"
#include "ShaderProgram.h"
#include "TriangleBvh.h"

class Camera;

// The ray-traced view (F9; docs/objects/ray-tracing.md). A full-screen pass
// in which every pixel fires a primary ray from the camera and intersects
// the analytic scene: body spheres, ring annuli and the emissive Sun. Hits
// are shaded with the same lights as the raster path, plus a shadow ray to
// the Sun (soft, area light), rays that continue through translucent rings,
// and one mirror-reflection bounce off reflective surfaces (Whitted-style).
//
// It is hybrid: the pass writes logarithmic depth, so rasterised geometry
// the tracer does not model (Voyager 2, orbit lines, belts) still composites
// correctly in front of or behind the traced worlds.
class RayTracer
{
public:
	RayTracer() = default;
	~RayTracer();
	RayTracer(const RayTracer&) = delete;
	RayTracer& operator=(const RayTracer&) = delete;

	bool initialize();

	// Albedo images in TraceSphere::textureLayer order. The texture array is
	// built lazily on the first traced frame so startup is not slowed.
	void setTexturePaths(std::vector<std::string> paths) { m_texturePaths = std::move(paths); }

	// Draws the traced worlds over the current frame buffer (after the
	// opaque raster pass, before translucent geometry).
	void render(const Camera& camera, float aspectRatio, const RayTraceScene& scene,
		const LightingState& lighting, float farPlane);

	// Voyager's triangles, traced by the same pass (see TriangleBvh).
	void setTracedMesh(const TriangleBvh* bvh, const glm::dmat4& worldMatrix, double boundingRadius)
	{
		m_tracedMesh = bvh;
		m_tracedMeshWorld = worldMatrix;
		m_tracedMeshRadius = boundingRadius;
	}

	// Reflection bounces per primary ray (0 = shadow rays only).
	void setMaxBounces(int bounces) { m_maxBounces = bounces; }
	int maxBounces() const { return m_maxBounces; }

private:
	void buildAtlas();

	ShaderProgram m_program;
	GLuint m_emptyVao = 0;        // the full-screen triangle comes from gl_VertexID
	GLuint m_atlas = 0;           // GL_TEXTURE_2D_ARRAY of every body's albedo
	bool m_atlasBuilt = false;
	std::vector<std::string> m_texturePaths;
	int m_maxBounces = 1;
	const TriangleBvh* m_tracedMesh = nullptr;
	glm::dmat4 m_tracedMeshWorld{ 1.0 };
	double m_tracedMeshRadius = 0.0;

	static constexpr int kAtlasWidth = 1024;
	static constexpr int kAtlasHeight = 512;
};

#endif
