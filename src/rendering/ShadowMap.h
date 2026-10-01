#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "ShaderProgram.h"

class Mesh;

// Shadow mapping for Voyager 2's self-shadowing (docs/objects/lighting.md,
// guide chapter 7). Once per frame the spacecraft is drawn from the Sun into
// a depth-only texture; each spacecraft fragment then compares its own depth
// from the Sun with the stored nearest depth. This replaced a per-pixel ray
// traced through Voyager's BVH, which cost ~10 ms per close-up frame; the
// depth pass plus a few filtered lookups costs a fraction of a millisecond.
//
// Everything is in the camera-relative frame (floating origin). Move-only
// RAII owner of its framebuffer and depth texture.
class ShadowMap
{
public:
	struct Caster
	{
		const Mesh* mesh = nullptr;
		glm::dmat4 world{ 1.0 };
	};

	ShadowMap() = default;
	~ShadowMap();
	ShadowMap(const ShadowMap&) = delete;
	ShadowMap& operator=(const ShadowMap&) = delete;

	bool initialize(int size = 2048);

	// Renders `casters` (world matrices) as seen from the Sun, fitted to the
	// sphere (centre, radius). `origin` is this frame's camera position.
	// Restores the default framebuffer and the given viewport afterwards.
	void render(const std::vector<Caster>& casters, const glm::dvec3& centre, double radius,
		const glm::dvec3& sunPosition, const glm::dvec3& origin, int viewportWidth, int viewportHeight);

	// Camera-relative position -> shadow texture (xy) and depth (z), all 0..1.
	const glm::mat4& matrix() const { return m_matrix; }
	bool ready() const { return m_ready; }
	void invalidate() { m_ready = false; }
	// Binds the depth texture (comparison sampler) to `unit`.
	void bind(GLuint unit) const;
	int size() const { return m_size; }

private:
	void release();

	ShaderProgram m_program;
	GLuint m_framebuffer = 0;
	GLuint m_depthTexture = 0;
	int m_size = 0;
	glm::mat4 m_matrix{ 1.0f };
	bool m_ready = false;
};

#endif
