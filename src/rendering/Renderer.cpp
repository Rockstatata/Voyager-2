#include "Renderer.h"

#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"

namespace
{
	GLint location(GLuint program, const char* name)
	{
		return glGetUniformLocation(program, name);
	}
}

void Renderer::setShader(Shader* shader)
{
	m_shader = shader;
	cacheUniformLocations();
}

void Renderer::cacheUniformLocations()
{
	m_cachedShaderID = m_shader != nullptr ? m_shader->ID : 0;
	const GLuint program = m_cachedShaderID;
	if (program == 0)
		return;

	m_modelLocation = location(program, "model");
	m_viewLocation = location(program, "view");
	m_projLocation = location(program, "proj");
	m_baseColorLocation = location(program, "baseColor");
	m_useTextureLocation = location(program, "useTexture");
	m_albedoTextureLocation = location(program, "albedoTexture");
	m_useInstancingLocation = location(program, "useInstancing");
	m_shadingLocation = location(program, "shadingModel");
	m_lightPositionLocation = location(program, "lightPosition");
	m_lightColorLocation = location(program, "lightColor");
	m_lightingEnabledLocation = location(program, "lightingEnabled");
	m_specularStrengthLocation = location(program, "specularStrength");
	m_specularPowerLocation = location(program, "specularPower");
	m_opacityLocation = location(program, "opacity");
	m_logDepthCoefficientLocation = location(program, "logDepthCoefficient");
}

void Renderer::setLight(const glm::dvec3& worldPosition, const glm::vec3& color)
{
	m_lightWorldPosition = worldPosition;
	m_lightColor = color;
}

void Renderer::beginFrame(const Camera& camera, float aspectRatio)
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glDisable(GL_BLEND);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_deferred.clear();

	if (m_shader == nullptr)
		return;
	if (m_shader->ID != m_cachedShaderID)
		cacheUniformLocations();

	m_shader->Activate();
	m_origin = camera.position();

	const glm::mat4 view = camera.viewMatrixAtOrigin();
	const glm::mat4 proj = camera.projectionMatrix(aspectRatio);
	const glm::vec3 lightRelative(m_lightWorldPosition - m_origin);

	glUniformMatrix4fv(m_viewLocation, 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(m_projLocation, 1, GL_FALSE, &proj[0][0]);
	glUniform1i(m_albedoTextureLocation, 0);
	glUniform3fv(m_lightPositionLocation, 1, &lightRelative[0]);
	glUniform3fv(m_lightColorLocation, 1, &m_lightColor[0]);
	glUniform1i(m_lightingEnabledLocation, m_lightingEnabled ? 1 : 0);
	glUniform1f(m_logDepthCoefficientLocation, 2.0f / std::log2(kFarPlane + 1.0f));
}

void Renderer::applyMaterial(const Material& material)
{
	glUniform3fv(m_baseColorLocation, 1, &material.baseColor[0]);
	glUniform1i(m_shadingLocation, static_cast<int>(material.shading));
	glUniform1f(m_specularStrengthLocation, material.specularStrength);
	glUniform1f(m_specularPowerLocation, material.specularPower);
	glUniform1f(m_opacityLocation, material.opacity);

	const bool hasTexture = material.albedoTexture != nullptr && material.albedoTexture->valid();
	glUniform1i(m_useTextureLocation, hasTexture ? 1 : 0);
	if (hasTexture)
		material.albedoTexture->bind(0);
}

void Renderer::submit(const Mesh& mesh, const Material& material, const glm::dmat4& worldMatrix)
{
	if (m_shader == nullptr)
		return;

	glm::dmat4 relative = worldMatrix;
	relative[3] -= glm::dvec4(m_origin, 0.0);
	const glm::mat4 model(relative);

	if (material.shading == ShadingModel::Glow || material.opacity < 1.0f)
	{
		m_deferred.push_back({ &mesh, &material, model });
		return;
	}

	glUniform1i(m_useInstancingLocation, 0);
	glUniformMatrix4fv(m_modelLocation, 1, GL_FALSE, &model[0][0]);
	applyMaterial(material);
	mesh.draw();
}

void Renderer::submitInstanced(const Mesh& mesh, const Material& material)
{
	if (m_shader == nullptr)
		return;

	// Instance matrices hold world positions; the shared model uniform moves
	// them into the camera-relative frame (model * instance in the shader).
	const glm::mat4 originShift = glm::translate(glm::mat4(1.0f), glm::vec3(-m_origin));
	glUniform1i(m_useInstancingLocation, 1);
	glUniformMatrix4fv(m_modelLocation, 1, GL_FALSE, &originShift[0][0]);
	applyMaterial(material);
	mesh.drawInstanced();
}

void Renderer::submitBackground(const Mesh& mesh, const Material& material)
{
	if (m_shader == nullptr)
		return;

	const glm::mat4 model(1.0f); // centred on the camera by construction
	glDepthMask(GL_FALSE);
	glUniform1i(m_useInstancingLocation, 0);
	glUniformMatrix4fv(m_modelLocation, 1, GL_FALSE, &model[0][0]);
	applyMaterial(material);
	mesh.draw();
	glDepthMask(GL_TRUE);
}

void Renderer::endFrame()
{
	if (m_shader == nullptr || m_deferred.empty())
		return;

	// Halos and translucent sheets: depth-tested against the opaque scene but
	// never written, so they cannot hide each other or what lies behind.
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);
	glUniform1i(m_useInstancingLocation, 0);
	for (const DeferredDraw& draw : m_deferred)
	{
		if (draw.material->shading == ShadingModel::Glow)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		else
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glUniformMatrix4fv(m_modelLocation, 1, GL_FALSE, &draw.model[0][0]);
		applyMaterial(*draw.material);
		draw.mesh->draw();
	}
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	m_deferred.clear();
}
