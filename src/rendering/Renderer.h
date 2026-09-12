#ifndef RENDERER_H
#define RENDERER_H

#include <glm/glm.hpp>

#include "../../shaderClass.h"
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

	// Per-object draw. Only the model matrix changes between submits.
	void submit(const Mesh& mesh, const glm::mat4& modelMatrix);

	void endFrame();

	void setClearColor(const glm::vec4& color) { m_clearColor = color; }

private:
	Shader* m_shader = nullptr; // non-owning; the Application owns the shader

	glm::vec4 m_clearColor{ 0.102f, 0.137f, 0.494f, 1.0f };

	// Uniform locations are resolved once per shader instead of per draw call.
	void cacheUniformLocations();
	GLuint m_cachedShaderID = 0;
	GLint m_modelLocation = -1;
	GLint m_viewLocation = -1;
	GLint m_projLocation = -1;
	GLint m_scaleLocation = -1;
};

#endif
