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
		m_modelLocation = m_viewLocation = m_projLocation = -1;
		m_baseColorLocation = m_useTextureLocation = m_albedoTextureLocation = -1;
		m_useInstancingLocation = -1;
		return;
	}

	m_cachedShaderID = m_shader->ID;
	m_modelLocation = glGetUniformLocation(m_shader->ID, "model");
	m_viewLocation = glGetUniformLocation(m_shader->ID, "view");
	m_projLocation = glGetUniformLocation(m_shader->ID, "proj");
	m_baseColorLocation = glGetUniformLocation(m_shader->ID, "baseColor");
	m_useTextureLocation = glGetUniformLocation(m_shader->ID, "useTexture");
	m_albedoTextureLocation = glGetUniformLocation(m_shader->ID, "albedoTexture");
	m_useInstancingLocation = glGetUniformLocation(m_shader->ID, "useInstancing");
}

void Renderer::beginFrame(const Camera& camera, float aspectRatio)
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glPointSize(2.0f); // fixed size; no per-vertex gl_PointSize needed for the starfield
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

	if (m_albedoTextureLocation >= 0)
		glUniform1i(m_albedoTextureLocation, 0);
}

void Renderer::submit(const Mesh& mesh, const Material& material,
					  const glm::mat4& modelMatrix)
{
	if (m_shader == nullptr)
		return;

	if (m_useInstancingLocation >= 0)
		glUniform1i(m_useInstancingLocation, 0);
	if (m_modelLocation >= 0)
		glUniformMatrix4fv(m_modelLocation, 1, GL_FALSE, &modelMatrix[0][0]);
	if (m_baseColorLocation >= 0)
		glUniform3fv(m_baseColorLocation, 1, &material.baseColor[0]);

	const bool hasTexture =
		material.albedoTexture != nullptr && material.albedoTexture->valid();
	if (m_useTextureLocation >= 0)
		glUniform1i(m_useTextureLocation, hasTexture ? 1 : 0);
	if (hasTexture)
		material.albedoTexture->bind(0);

	mesh.draw();
}

void Renderer::submitInstanced(const Mesh& mesh, const Material& material)
{
	if (m_shader == nullptr)
		return;

	if (m_useInstancingLocation >= 0)
		glUniform1i(m_useInstancingLocation, 1);
	if (m_baseColorLocation >= 0)
		glUniform3fv(m_baseColorLocation, 1, &material.baseColor[0]);

	const bool hasTexture =
		material.albedoTexture != nullptr && material.albedoTexture->valid();
	if (m_useTextureLocation >= 0)
		glUniform1i(m_useTextureLocation, hasTexture ? 1 : 0);
	if (hasTexture)
		material.albedoTexture->bind(0);

	mesh.drawInstanced();
}

void Renderer::endFrame()
{
	// Buffer swap belongs to Window, driven by Application::run().
}
