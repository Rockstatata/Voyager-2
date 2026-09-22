#ifndef RENDERER_H
#define RENDERER_H

#include <glm/glm.hpp>

#include "../../shaderClass.h"
#include "Material.h"
#include "Mesh.h"

class Camera;

// Owns OpenGL state policy and the per-frame draw path (bible section 14).
// It holds no camera of its own: the camera is passed in each frame, so
// moving the camera can never be confused with changing render state.
class Renderer
{
public:
	void setShader(Shader* shader);

	// Clears the frame and uploads the camera's view/projection once.
	void beginFrame(const Camera& camera, float aspectRatio);

	// Per-object draw. Geometry, appearance and placement remain independent.
	void submit(const Mesh& mesh, const Material& material,
				const glm::mat4& modelMatrix);

	// Draws every instance Mesh::setInstanceTransforms uploaded, in one GPU
	// call — the belt/starfield path. No single modelMatrix: each instance
	// carries its own (see Mesh.h / default.vert).
	void submitInstanced(const Mesh& mesh, const Material& material);

	void endFrame();

	void setClearColor(const glm::vec4& color) { m_clearColor = color; }

private:
	Shader* m_shader = nullptr; // non-owning; the Application owns the shader

	// Pitch-black space (bible section 30 background) rather than the
	// starter project's placeholder blue; a starfield is layered in later.
	glm::vec4 m_clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };

	// Uniform locations are resolved once per shader instead of per draw call.
	void cacheUniformLocations();
	GLuint m_cachedShaderID = 0;
	GLint m_modelLocation = -1;
	GLint m_viewLocation = -1;
	GLint m_projLocation = -1;
	GLint m_baseColorLocation = -1;
	GLint m_useTextureLocation = -1;
	GLint m_albedoTextureLocation = -1;
	GLint m_useInstancingLocation = -1;
};

#endif
