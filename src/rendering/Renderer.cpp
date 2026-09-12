#include "Renderer.h"

#include "Camera.h"

void Renderer::setShader(Shader* shader)
{
	m_shader = shader;
	cacheUniformLocations();
}

void Renderer::cacheUniformLocations()
{
	if (m_shader == nullptr)
	{
		m_cachedShaderID = 0;
		m_modelLocation = m_viewLocation = m_projLocation = m_scaleLocation = -1;
		return;
	}

	m_cachedShaderID = m_shader->ID;
	m_modelLocation = glGetUniformLocation(m_shader->ID, "model");
	m_viewLocation = glGetUniformLocation(m_shader->ID, "view");
	m_projLocation = glGetUniformLocation(m_shader->ID, "proj");
	m_scaleLocation = glGetUniformLocation(m_shader->ID, "scale");
}

void Renderer::beginFrame(const Camera& camera, float aspectRatio)
{
	glEnable(GL_DEPTH_TEST);
	glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (m_shader == nullptr)
		return;

	if (m_shader->ID != m_cachedShaderID)
		cacheUniformLocations();

	m_shader->Activate();

	const glm::mat4 view = camera.viewMatrix();
	const glm::mat4 proj = camera.projectionMatrix(aspectRatio);

	if (m_viewLocation >= 0)
		glUniformMatrix4fv(m_viewLocation, 1, GL_FALSE, &view[0][0]);
	if (m_projLocation >= 0)
		glUniformMatrix4fv(m_projLocation, 1, GL_FALSE, &proj[0][0]);

	// The starter shader's leftover uniform; the model matrix carries scale
	// now, so this stays neutral until the shader lab replaces it.
	if (m_scaleLocation >= 0)
		glUniform1f(m_scaleLocation, 1.0f);
}

void Renderer::submit(const Mesh& mesh, const glm::mat4& modelMatrix)
{
	if (m_shader == nullptr)
		return;

	if (m_modelLocation >= 0)
		glUniformMatrix4fv(m_modelLocation, 1, GL_FALSE, &modelMatrix[0][0]);

	mesh.draw();
}

void Renderer::endFrame()
{
	// Buffer swap belongs to Window, driven by Application::run().
}
