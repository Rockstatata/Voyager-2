#include "Renderer.h"

#include <algorithm>
#include <cmath>
#include <string>

#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"

bool Renderer::initialize()
{
	return m_program.load("shaders/scene.vert", "shaders/scene.frag");
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

	if (!m_program.valid())
		return;
	m_program.use();
	m_origin = camera.position();

	const glm::mat4 view = camera.viewMatrixAtOrigin();
	const glm::mat4 proj = camera.projectionMatrix(aspectRatio);
	glUniformMatrix4fv(m_program.uniform("view"), 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(m_program.uniform("proj"), 1, GL_FALSE, &proj[0][0]);
	glUniform1i(m_program.uniform("albedoTexture"), 0);
	glUniform1i(m_program.uniform("normalMap"), 1);
	glUniform1i(m_program.uniform("specularMap"), 2);
	glUniform1f(m_program.uniform("logDepthCoefficient"), 2.0f / std::log2(kFarPlane + 1.0f));
	uploadLights();
	uploadTraceScene(m_program);
}

void Renderer::uploadTraceScene(ShaderProgram& program) const
{
	const int sphereCount = std::min(static_cast<int>(m_traceScene.spheres.size()), RayTraceScene::kMaxSpheres);
	glUniform1i(program.uniform("sphereCount"), sphereCount);
	for (int i = 0; i < sphereCount; ++i)
	{
		const TraceSphere& sphere = m_traceScene.spheres[i];
		const glm::vec4 packed(glm::vec3(sphere.center - m_origin), static_cast<float>(sphere.radius));
		const std::string index = "[" + std::to_string(i) + "]";
		glUniform4fv(program.uniform("spheres" + index), 1, &packed[0]);
		glUniform1i(program.uniform("sphereEmissive" + index), sphere.emissive ? 1 : 0);
	}

	const int ringCount = std::min(static_cast<int>(m_traceScene.rings.size()), RayTraceScene::kMaxRings);
	glUniform1i(program.uniform("ringCount"), ringCount);
	for (int i = 0; i < ringCount; ++i)
	{
		const TraceRing& ring = m_traceScene.rings[i];
		const glm::vec4 center(glm::vec3(ring.center - m_origin), static_cast<float>(ring.innerRadius));
		const glm::vec4 normal(ring.normal, static_cast<float>(ring.outerRadius));
		const std::string index = "[" + std::to_string(i) + "]";
		glUniform4fv(program.uniform("ringCenters" + index), 1, &center[0]);
		glUniform4fv(program.uniform("ringNormals" + index), 1, &normal[0]);
		glUniform1f(program.uniform("ringOpacity" + index), ring.opacity);
		glUniform3fv(program.uniform("ringColors" + index), 1, &ring.color[0]);
	}

	const glm::vec3 sunCenter(m_traceScene.sunPosition - m_origin);
	glUniform3fv(program.uniform("sunCenter"), 1, &sunCenter[0]);
	glUniform1f(program.uniform("sunLightRadius"), static_cast<float>(m_traceScene.sunLightRadius));
	glUniform1i(program.uniform("shadowMode"), m_lighting.enabled ? static_cast<int>(m_lighting.shadows) : 0);
}

void Renderer::uploadLights()
{
	glUniform1i(m_program.uniform("lightingEnabled"), m_lighting.enabled ? 1 : 0);
	glUniform1i(m_program.uniform("shadingTechnique"), static_cast<int>(m_lighting.technique));
	glUniform1f(m_program.uniform("ambientStrength"), m_lighting.ambient);
	glUniform1i(m_program.uniform("surfaceMapsEnabled"), m_lighting.surfaceMaps ? 1 : 0);

	for (int i = 0; i < kMaxLights; ++i)
	{
		const Light& light = m_lighting.lights[i];
		const std::string prefix = "lights[" + std::to_string(i) + "].";
		// Camera-relative, computed in double before narrowing.
		const glm::vec3 relativePosition(light.position - m_origin);
		const glm::vec3 radiance = light.color * light.intensity;
		const glm::vec3 direction = glm::length(light.direction) > 0.0f ? glm::normalize(light.direction)
			: glm::vec3(0.0f, -1.0f, 0.0f);
		glUniform1i(m_program.uniform(prefix + "type"), static_cast<int>(light.type));
		glUniform1i(m_program.uniform(prefix + "enabled"), light.enabled ? 1 : 0);
		glUniform3fv(m_program.uniform(prefix + "position"), 1, &relativePosition[0]);
		glUniform3fv(m_program.uniform(prefix + "direction"), 1, &direction[0]);
		glUniform3fv(m_program.uniform(prefix + "color"), 1, &radiance[0]);
		glUniform3fv(m_program.uniform(prefix + "attenuation"), 1, &light.attenuation[0]);
		glUniform1f(m_program.uniform(prefix + "innerCutoff"), light.innerCutoffCos);
		glUniform1f(m_program.uniform(prefix + "outerCutoff"), light.outerCutoffCos);
	}
}

void Renderer::applyMaterial(const Material& material)
{
	glUniform3fv(m_program.uniform("baseColor"), 1, &material.baseColor[0]);
	glUniform1i(m_program.uniform("shadingModel"), static_cast<int>(material.shading));
	glUniform1f(m_program.uniform("specularStrength"), material.specularStrength);
	glUniform1f(m_program.uniform("specularPower"), material.specularPower);
	glUniform1f(m_program.uniform("opacity"), material.opacity);

	const bool hasTexture = material.albedoTexture != nullptr && material.albedoTexture->valid();
	glUniform1i(m_program.uniform("useTexture"), hasTexture ? 1 : 0);
	if (hasTexture)
		material.albedoTexture->bind(0);

	const bool hasNormalMap = material.normalTexture != nullptr && material.normalTexture->valid();
	glUniform1i(m_program.uniform("useNormalMap"), hasNormalMap ? 1 : 0);
	glUniform1f(m_program.uniform("normalStrength"), material.normalStrength);
	if (hasNormalMap)
		material.normalTexture->bind(1);
	const bool hasSpecularMap = material.specularTexture != nullptr && material.specularTexture->valid();
	glUniform1i(m_program.uniform("useSpecularMap"), hasSpecularMap ? 1 : 0);
	if (hasSpecularMap)
		material.specularTexture->bind(2);
}

void Renderer::submit(const Mesh& mesh, const Material& material, const glm::dmat4& worldMatrix)
{
	if (!m_program.valid())
		return;

	glm::dmat4 relative = worldMatrix;
	relative[3] -= glm::dvec4(m_origin, 0.0);
	const glm::mat4 model(relative);

	if (material.shading == ShadingModel::Glow || material.opacity < 1.0f)
	{
		m_deferred.push_back({ &mesh, &material, model });
		return;
	}

	glUniform1i(m_program.uniform("useInstancing"), 0);
	glUniformMatrix4fv(m_program.uniform("model"), 1, GL_FALSE, &model[0][0]);
	applyMaterial(material);
	mesh.draw();
}

void Renderer::submitInstanced(const Mesh& mesh, const Material& material)
{
	if (!m_program.valid())
		return;

	// Instance matrices hold world positions; the shared model uniform moves
	// them into the camera-relative frame (model * instance in the shader).
	const glm::mat4 originShift = glm::translate(glm::mat4(1.0f), glm::vec3(-m_origin));
	glUniform1i(m_program.uniform("useInstancing"), 1);
	glUniformMatrix4fv(m_program.uniform("model"), 1, GL_FALSE, &originShift[0][0]);
	applyMaterial(material);
	mesh.drawInstanced();
}

void Renderer::submitBackground(const Mesh& mesh, const Material& material)
{
	if (!m_program.valid())
		return;

	const glm::mat4 model(1.0f); // centred on the camera by construction
	glDepthMask(GL_FALSE);
	glUniform1i(m_program.uniform("useInstancing"), 0);
	glUniformMatrix4fv(m_program.uniform("model"), 1, GL_FALSE, &model[0][0]);
	applyMaterial(material);
	mesh.draw();
	glDepthMask(GL_TRUE);
}

void Renderer::endFrame()
{
	if (!m_program.valid() || m_deferred.empty())
		return;

	// Halos and translucent sheets: depth-tested against the opaque scene but
	// never written, so they cannot hide each other or what lies behind.
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);
	glUniform1i(m_program.uniform("useInstancing"), 0);
	for (const DeferredDraw& draw : m_deferred)
	{
		if (draw.material->shading == ShadingModel::Glow)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		else
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glUniformMatrix4fv(m_program.uniform("model"), 1, GL_FALSE, &draw.model[0][0]);
		applyMaterial(*draw.material);
		draw.mesh->draw();
	}
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	m_deferred.clear();
}
