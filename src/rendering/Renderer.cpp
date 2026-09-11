#include "Renderer.h"

#include <glm/gtc/matrix_transform.hpp>

void Renderer::setShader(Shader* shader)
{
	m_shader = shader;
}

void Renderer::setViewProjection(const glm::mat4& view, const glm::mat4& proj)
{
	m_view = view;
	m_proj = proj;
	m_gotProj = true;
}

void Renderer::beginFrame()
{
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.102f, 0.137f, 0.494f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Default static camera (temporary until a Camera system exists).
	if (!m_gotProj)
	{
		m_view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.0f));
		m_proj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
	}

	if (m_shader != nullptr)
	{
		m_shader->Activate();
		glUniformMatrix4fv(glGetUniformLocation(m_shader->ID, "view"), 1, GL_FALSE, &m_view[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(m_shader->ID, "proj"), 1, GL_FALSE, &m_proj[0][0]);
		glUniform1f(glGetUniformLocation(m_shader->ID, "scale"), 0.5f);
	}
}

void Renderer::submit(const Mesh& mesh, const glm::mat4& modelMatrix)
{
	if (m_shader == nullptr)
		return;
	glUniformMatrix4fv(glGetUniformLocation(m_shader->ID, "model"), 1, GL_FALSE, &modelMatrix[0][0]);
	mesh.draw();
}

void Renderer::endFrame()
{
	// Buffer swap happens in the application loop (Main.cpp).
}
