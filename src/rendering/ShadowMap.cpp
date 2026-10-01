#include "ShadowMap.h"

#include <cmath>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include "Mesh.h"

ShadowMap::~ShadowMap()
{
	release();
}

void ShadowMap::release()
{
	if (m_depthTexture != 0)
		glDeleteTextures(1, &m_depthTexture);
	if (m_framebuffer != 0)
		glDeleteFramebuffers(1, &m_framebuffer);
	m_depthTexture = 0;
	m_framebuffer = 0;
}

bool ShadowMap::initialize(int size)
{
	if (!m_program.load("shaders/shadow.vert", "shaders/shadow.frag"))
		return false;
	m_size = size;

	glGenTextures(1, &m_depthTexture);
	glBindTexture(GL_TEXTURE_2D, m_depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	// Comparison sampling: texture() returns the lit fraction directly, and
	// GL_LINEAR blends four comparisons (hardware 2x2 percentage-closer filtering).
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	const float border[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; // outside the map: lit
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &m_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	if (!complete)
	{
		std::cout << "[OPENGL] shadow-map framebuffer incomplete" << std::endl;
		release();
		return false;
	}
	std::cout << "[SHADER] shadow map " << size << "x" << size << " ready" << std::endl;
	return true;
}

void ShadowMap::render(const std::vector<Caster>& casters, const glm::dvec3& centre, double radius,
	const glm::dvec3& sunPosition, const glm::dvec3& origin, int viewportWidth, int viewportHeight)
{
	m_ready = false;
	if (m_framebuffer == 0 || !m_program.valid() || casters.empty() || radius <= 0.0)
		return;

	// The light camera: orthographic (the Sun is effectively a directional
	// light across a 20 m spacecraft), looking from the Sun at the caster
	// sphere, its box exactly fitting the sphere.
	const glm::dvec3 toSun = glm::normalize(sunPosition - centre);
	const glm::vec3 target(centre - origin);
	const glm::vec3 eye(centre + toSun * (radius * 2.0) - origin);
	glm::vec3 up(0.0f, 1.0f, 0.0f);
	if (std::abs(glm::dot(glm::vec3(toSun), up)) > 0.99f)
		up = glm::vec3(1.0f, 0.0f, 0.0f);
	const float r = static_cast<float>(radius);
	const glm::mat4 lightView = glm::lookAt(eye, target, up);
	const glm::mat4 lightProjection = glm::ortho(-r, r, -r, r, 0.0f, 4.0f * r);
	const glm::mat4 lightMatrix = lightProjection * lightView;
	// [-1, 1] clip space -> [0, 1] texture coordinates and depth.
	const glm::mat4 toTexture = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f)) *
		glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
	m_matrix = toTexture * lightMatrix;

	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
	glViewport(0, 0, m_size, m_size);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glClear(GL_DEPTH_BUFFER_BIT);
	// Thin open parts (rods, blankets) must cast from either face.
	glDisable(GL_CULL_FACE);
	// Slope-scaled depth bias: surfaces at a grazing angle to the Sun need
	// more offset, or they shadow themselves in stripes ("shadow acne").
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.0f, 4.0f);

	m_program.use();
	glUniformMatrix4fv(m_program.uniform("lightMatrix"), 1, GL_FALSE, &lightMatrix[0][0]);
	const GLint modelLocation = m_program.uniform("model");
	for (const Caster& caster : casters)
	{
		glm::dmat4 relative = caster.world;
		relative[3] -= glm::dvec4(origin, 0.0);
		const glm::mat4 model(relative);
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &model[0][0]);
		caster.mesh->draw();
	}

	glDisable(GL_POLYGON_OFFSET_FILL);
	glEnable(GL_CULL_FACE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, viewportWidth, viewportHeight);
	m_ready = true;
}

void ShadowMap::bind(GLuint unit) const
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, m_depthTexture);
	glActiveTexture(GL_TEXTURE0);
}
