#ifndef RENDERER_H
#define RENDERER_H

#include <vector>

#include <glm/glm.hpp>

#include "Material.h"
#include "Mesh.h"
#include "ShaderProgram.h"

class Camera;

// Owns OpenGL state policy and the per-frame draw path (bible section 14).
// It holds no camera of its own: the camera is passed in each frame, so
// moving the camera can never be confused with changing render state.
//
// Floating origin (bible section 20): every world matrix arrives in double
// precision and has the camera position subtracted BEFORE it is narrowed to
// float. The GPU therefore only ever sees camera-relative coordinates, so a
// 13 m antenna strut stays stable even hundreds of units from the Sun.
//
// Depth uses a logarithmic mapping in the shaders, which lets one frame keep
// both a 2 cm spacecraft detail and the 3,000-unit Oort cloud without
// z-fighting or clipping — no per-mode near/far juggling.
class Renderer
{
public:
	// Loads shaders/scene.vert + scene.frag. False if they fail to build.
	bool initialize();

	// Clears the frame, uploads the camera's view/projection and the Sun light.
	void beginFrame(const Camera& camera, float aspectRatio);

	// Sun position in world space; lights every Lit material.
	void setLight(const glm::dvec3& worldPosition, const glm::vec3& color);
	void setLightingEnabled(bool enabled) { m_lightingEnabled = enabled; }
	bool lightingEnabled() const { return m_lightingEnabled; }

	// Per-object draw. Geometry, appearance and placement remain independent.
	// Glow materials are queued and drawn additively in endFrame().
	void submit(const Mesh& mesh, const Material& material, const glm::dmat4& worldMatrix);

	// Draws every instance Mesh::setInstanceTransforms uploaded, in one GPU
	// call. Instance matrices are world-space; the floating origin is applied
	// through the shared `model` uniform.
	void submitInstanced(const Mesh& mesh, const Material& material);

	// Camera-centred background (starfield): drawn without depth writes, so
	// everything else is always in front of it regardless of its radius.
	void submitBackground(const Mesh& mesh, const Material& material);

	void endFrame();

	void setClearColor(const glm::vec4& color) { m_clearColor = color; }
	const glm::dvec3& origin() const { return m_origin; }

	static constexpr float kFarPlane = 1.0e6f;

private:
	struct DeferredDraw
	{
		const Mesh* mesh = nullptr;
		const Material* material = nullptr;
		glm::mat4 model{ 1.0f };
	};

	void applyMaterial(const Material& material);
	void cacheUniformLocations();

	ShaderProgram m_program;
	glm::vec4 m_clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
	glm::dvec3 m_origin{ 0.0 };
	glm::dvec3 m_lightWorldPosition{ 0.0 };
	glm::vec3 m_lightColor{ 1.0f };
	bool m_lightingEnabled = true;
	std::vector<DeferredDraw> m_deferred;

	GLint m_modelLocation = -1;
	GLint m_viewLocation = -1;
	GLint m_projLocation = -1;
	GLint m_baseColorLocation = -1;
	GLint m_useTextureLocation = -1;
	GLint m_albedoTextureLocation = -1;
	GLint m_useInstancingLocation = -1;
	GLint m_shadingLocation = -1;
	GLint m_lightPositionLocation = -1;
	GLint m_lightColorLocation = -1;
	GLint m_lightingEnabledLocation = -1;
	GLint m_specularStrengthLocation = -1;
	GLint m_specularPowerLocation = -1;
	GLint m_opacityLocation = -1;
	GLint m_logDepthCoefficientLocation = -1;
};

#endif
