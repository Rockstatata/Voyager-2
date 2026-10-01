#include "Renderer.h"

#include <algorithm>
#include <cmath>
#include <string>

#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"
#include "LightingUniforms.h"

bool Renderer::initialize()
{
	if (!m_program.load("shaders/scene.vert", "shaders/scene.frag"))
		return false;
	cacheUniformLocations();
	// A missing shadow map only loses Voyager's self-shadows.
	m_shadowMap.initialize(kShadowMapSize);
	return true;
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
	m_stats = {};

	if (!m_program.valid())
		return;
	m_program.use();
	m_origin = camera.position();

	const glm::mat4 view = camera.viewMatrixAtOrigin();
	const glm::mat4 proj = camera.projectionMatrix(aspectRatio);

	// Frustum planes from the combined matrix (Gribb and Hartmann): each is
	// a row sum, in the camera-relative frame the shaders also use.
	const glm::mat4 clip = proj * view;
	for (int axis = 0; axis < 3; ++axis)
	{
		for (int side = 0; side < 2; ++side)
		{
			glm::vec4 plane;
			for (int column = 0; column < 4; ++column)
				plane[column] = clip[column][3] + (side == 0 ? 1.0f : -1.0f) * clip[column][axis];
			m_frustum[axis * 2 + side] = plane / glm::length(glm::vec3(plane));
		}
	}
	m_shadowMap.invalidate();
	glUniformMatrix4fv(m_program.uniform("view"), 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(m_program.uniform("proj"), 1, GL_FALSE, &proj[0][0]);
	glUniform1i(m_program.uniform("albedoTexture"), 0);
	glUniform1i(m_program.uniform("normalMap"), 1);
	glUniform1i(m_program.uniform("specularMap"), 2);
	glUniform1f(m_program.uniform("logDepthCoefficient"), 2.0f / std::log2(kFarPlane + 1.0f));
	LightingUniforms::uploadLights(m_program, m_lighting, m_origin);
	LightingUniforms::uploadTraceScene(m_program, m_traceScene,
		m_lighting.enabled ? m_lighting.shadows : ShadowMode::Off, m_origin);
	glUniform1i(m_program.uniform("shadowMap"), kShadowMapUnit);
	// The raster pass never traces Voyager's BVH (the shadow map replaced it);
	// keep its buffer samplers on their own units so no unit holds two types.
	glUniform1i(m_program.uniform("meshEnabled"), 0);
	glUniform1i(m_program.uniform("bvhNodes"), 3);
	glUniform1i(m_program.uniform("bvhTriangles"), 4);
}

bool Renderer::isVisible(const glm::dvec3& centre, double radius) const
{
	const glm::vec3 relative(centre - m_origin);
	const float r = static_cast<float>(radius);
	for (const glm::vec4& plane : m_frustum)
	{
		if (glm::dot(glm::vec3(plane), relative) + plane.w < -r)
			return false;
	}
	return true;
}

void Renderer::renderShadowMap(const std::vector<ShadowMap::Caster>& casters, const glm::dvec3& centre,
	double radius, const glm::dvec3& sunPosition, int viewportWidth, int viewportHeight)
{
	if (!m_program.valid() || !m_lighting.enabled || m_lighting.shadows == ShadowMode::Off)
		return;
	m_shadowMap.render(casters, centre, radius, sunPosition, m_origin, viewportWidth, viewportHeight);
	m_program.use();
	if (!m_shadowMap.ready())
		return;
	m_shadowMap.bind(kShadowMapUnit);
	glUniformMatrix4fv(m_program.uniform("shadowMatrix"), 1, GL_FALSE, &m_shadowMap.matrix()[0][0]);
	// Texel size for the soft filter's sample spacing.
	glUniform1f(m_program.uniform("shadowTexel"), 1.0f / static_cast<float>(m_shadowMap.size()));
}

void Renderer::cacheUniformLocations()
{
	m_draw.model = m_program.uniform("model");
	m_draw.useInstancing = m_program.uniform("useInstancing");
	m_draw.baseColor = m_program.uniform("baseColor");
	m_draw.shadingModel = m_program.uniform("shadingModel");
	m_draw.specularStrength = m_program.uniform("specularStrength");
	m_draw.specularPower = m_program.uniform("specularPower");
	m_draw.opacity = m_program.uniform("opacity");
	m_draw.uvTransform = m_program.uniform("uvTransform");
	m_draw.selfShadowing = m_program.uniform("selfShadowing");
	m_draw.useTexture = m_program.uniform("useTexture");
	m_draw.useNormalMap = m_program.uniform("useNormalMap");
	m_draw.normalStrength = m_program.uniform("normalStrength");
	m_draw.useSpecularMap = m_program.uniform("useSpecularMap");
}

void Renderer::applyMaterial(const Material& material)
{
	glUniform3fv(m_draw.baseColor, 1, &material.baseColor[0]);
	glUniform1i(m_draw.shadingModel, static_cast<int>(material.shading));
	glUniform1f(m_draw.specularStrength, material.specularStrength);
	glUniform1f(m_draw.specularPower, material.specularPower);
	glUniform1f(m_draw.opacity, material.opacity);
	glUniform4fv(m_draw.uvTransform, 1, &material.uvTransform[0]);
	glUniform1i(m_draw.selfShadowing, material.selfShadowing && m_shadowMap.ready() ? 1 : 0);

	const bool hasTexture = material.albedoTexture != nullptr && material.albedoTexture->valid();
	glUniform1i(m_draw.useTexture, hasTexture ? 1 : 0);
	if (hasTexture)
		material.albedoTexture->bind(0);

	const bool hasNormalMap = material.normalTexture != nullptr && material.normalTexture->valid();
	glUniform1i(m_draw.useNormalMap, hasNormalMap ? 1 : 0);
	glUniform1f(m_draw.normalStrength, material.normalStrength);
	if (hasNormalMap)
		material.normalTexture->bind(1);
	const bool hasSpecularMap = material.specularTexture != nullptr && material.specularTexture->valid();
	glUniform1i(m_draw.useSpecularMap, hasSpecularMap ? 1 : 0);
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

	glUniform1i(m_draw.useInstancing, 0);
	glUniformMatrix4fv(m_draw.model, 1, GL_FALSE, &model[0][0]);
	applyMaterial(material);
	mesh.draw();
	++m_stats.drawCalls;
	m_stats.triangles += mesh.triangleCount();
}

void Renderer::submitInstanced(const Mesh& mesh, const Material& material)
{
	if (!m_program.valid())
		return;

	// Instance matrices hold world positions; the shared model uniform moves
	// them into the camera-relative frame (model * instance in the shader).
	const glm::mat4 originShift = glm::translate(glm::mat4(1.0f), glm::vec3(-m_origin));
	glUniform1i(m_draw.useInstancing, 1);
	glUniformMatrix4fv(m_draw.model, 1, GL_FALSE, &originShift[0][0]);
	applyMaterial(material);
	mesh.drawInstanced();
	++m_stats.drawCalls;
	m_stats.triangles += mesh.triangleCount(true);
}

void Renderer::submitBackground(const Mesh& mesh, const Material& material)
{
	if (!m_program.valid())
		return;

	const glm::mat4 model(1.0f); // centred on the camera by construction
	glDepthMask(GL_FALSE);
	glUniform1i(m_draw.useInstancing, 0);
	glUniformMatrix4fv(m_draw.model, 1, GL_FALSE, &model[0][0]);
	applyMaterial(material);
	mesh.draw();
	++m_stats.drawCalls;
	glDepthMask(GL_TRUE);
}

void Renderer::endFrame()
{
	if (!m_program.valid() || m_deferred.empty())
		return;

	// Halos and translucent sheets: depth-tested against the opaque scene but
	// never written, so they cannot hide each other or what lies behind.
	m_program.use(); // the ray tracer may have bound its own program
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);
	glUniform1i(m_draw.useInstancing, 0);
	for (const DeferredDraw& draw : m_deferred)
	{
		if (draw.material->shading == ShadingModel::Glow)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		else
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glUniformMatrix4fv(m_draw.model, 1, GL_FALSE, &draw.model[0][0]);
		applyMaterial(*draw.material);
		draw.mesh->draw();
		++m_stats.drawCalls;
		m_stats.triangles += draw.mesh->triangleCount();
	}
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	m_deferred.clear();
}
