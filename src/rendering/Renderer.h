#ifndef RENDERER_H
#define RENDERER_H

#include <glm/glm.hpp>

#include "../../shaderClass.h"
#include "Mesh.h"

// Owns OpenGL render state and the per-frame camera (view/projection) path.
// Objects submit mesh + model matrix; the renderer binds the shader,
// uploads uniforms, and draws. For now the camera is a simple static
// view/projection; a full Camera system arrives in a later phase.
class Renderer
{
public:
	void setShader(Shader* shader);
	void setViewProjection(const glm::mat4& view, const glm::mat4& proj);

	void beginFrame();
	void submit(const Mesh& mesh, const glm::mat4& modelMatrix);
	void endFrame();

private:
	Shader* m_shader = nullptr; // non-owning; shader outlives the frame
	glm::mat4 m_view{ 1.0f };
	glm::mat4 m_proj{ 1.0f };
	bool m_gotProj = false;
};

#endif
