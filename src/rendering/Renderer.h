#ifndef RENDERER_H
#define RENDERER_H

#include <vector>

#include <glm/glm.hpp>

#include "Lighting.h"
#include "Material.h"
#include "Mesh.h"
#include "RayTraceScene.h"
#include "ShaderProgram.h"

class Camera;

// Owns OpenGL state policy and the per-frame draw path (bible section 14).
// It holds no camera of its own: the camera is passed in each frame, so
// moving the camera can never be confused with changing render state.
//
// Floating origin (bible section 20): every world matrix arrives in double
// precision and has the camera position subtracted BEFORE it is narrowed to
// float. The GPU therefore only ever sees camera-relative coordinates.
//
// Depth uses a logarithmic mapping in the shaders, which lets one frame keep
// both a 2 cm spacecraft detail and the 3,000-unit Oort cloud without
// z-fighting or clipping.
//
// Lighting (Phase 14): one LightingState per frame supplies the shading
// technique, ambient level and up to four light casters; each Material then
// only says how its own surface responds (docs/objects/lighting.md).
class Renderer
{
public:
	// Loads shaders/scene.vert + scene.frag. False if they fail to build.
	bool initialize();

	void setLighting(const LightingState& lighting) { m_lighting = lighting; }
	// Spheres and ring bands the shadow rays are traced against.
	void setTraceScene(const RayTraceScene& scene) { m_traceScene = scene; }
	const RayTraceScene& traceScene() const { return m_traceScene; }
	const LightingState& lighting() const { return m_lighting; }

	// Clears the frame and uploads camera matrices and lights.
	void beginFrame(const Camera& camera, float aspectRatio);

	// Per-object draw. Geometry, appearance and placement remain independent.
	// Glow and translucent materials are queued and drawn in endFrame().
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
	void uploadLights();
	void uploadTraceScene(ShaderProgram& program) const;

	ShaderProgram m_program;
	glm::vec4 m_clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
	glm::dvec3 m_origin{ 0.0 };
	LightingState m_lighting;
	RayTraceScene m_traceScene;
	std::vector<DeferredDraw> m_deferred;
};

#endif
