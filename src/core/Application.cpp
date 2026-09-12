#include "Application.h"

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "../../shaderClass.h"
#include "../rendering/Mesh.h"

namespace
{
	// The starter quad: position (vec3) + colour (vec3), interleaved.
	const GLfloat kQuadVertices[] = {
		0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, //0
		1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, //1
		1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, //2
		0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f  //3
	};

	const GLuint kQuadIndices[] = {
		0, 1, 2,
		0, 3, 2
	};
}

Application::Application() = default;
Application::~Application() = default;

bool Application::initialize(int width, int height, const std::string& title)
{
	if (!m_window.create(width, height, title))
		return false;

	m_input.attach(m_window.handle());

	if (!loadShaders())
		return false;

	m_renderer.setShader(m_shader.get());

	// Reproduces the starter's hardcoded view: eye at +2 on Z looking down -Z.
	m_camera.setPosition(glm::dvec3(0.0, 0.0, 2.0));
	m_camera.setYawPitch(-90.0f, 0.0f);
	m_camera.setPerspective(45.0f, 0.1f, 100.0f);

	buildScene();

	std::cout << "[APP] initialized. RMB + WASD to fly, Space/Ctrl up-down, Shift boost, Esc to quit." << std::endl;

	m_initialized = true;
	return true;
}

bool Application::loadShaders()
{
	// Shader reads its files relative to the working directory, which must be
	// the project root. get_file_contents throws a bare errno on a missing file.
	try
	{
		m_shader = std::make_unique<Shader>("default.vert", "default.frag");
	}
	catch (...)
	{
		std::cout << "[SHADER] failed to read default.vert / default.frag. "
					 "The working directory must be the project root." << std::endl;
		return false;
	}

	GLint linked = GL_FALSE;
	glGetProgramiv(m_shader->ID, GL_LINK_STATUS, &linked);
	if (linked == GL_FALSE)
	{
		char log[1024] = {};
		glGetProgramInfoLog(m_shader->ID, sizeof(log), nullptr, log);
		std::cout << "[SHADER] program link failed:\n" << log << std::endl;
		return false;
	}

	return true;
}

void Application::buildScene()
{
	// Uploaded once; both objects point at the same GPU buffers.
	m_quadMesh = std::make_shared<Mesh>(
		kQuadVertices, sizeof(kQuadVertices),
		kQuadIndices, sizeof(kQuadIndices),
		(GLsizei)(sizeof(kQuadIndices) / sizeof(GLuint)));

	auto first = std::make_unique<SceneObject>("quad.primary");
	first->setMesh(m_quadMesh);
	// Exactly the starter's transform, so Phase 1 changed no pixels here.
	first->transform().position = glm::dvec3(-0.5, -0.5, 0.0);
	first->transform().rotation = glm::angleAxis(-45.0, glm::dvec3(0.0, 0.0, 1.0));
	m_firstQuad = &m_scene.addObject(std::move(first));

	// Phase 1 success test: a second object, same mesh, different transform.
	auto second = std::make_unique<SceneObject>("quad.secondary");
	second->setMesh(m_quadMesh);
	second->transform().position = glm::dvec3(0.6, -0.3, -0.5);
	second->transform().scale = glm::dvec3(0.6);
	m_secondQuad = &m_scene.addObject(std::move(second));

	std::cout << "[SCENE] 2 objects sharing 1 mesh" << std::endl;
}

void Application::run()
{
	if (!m_initialized)
	{
		std::cout << "[APP] run() called before a successful initialize()" << std::endl;
		return;
	}

	while (!m_window.shouldClose())
	{
		m_time.beginFrame(glfwGetTime());
		m_input.update();

		update(m_time.deltaTime());
		render();

		m_window.swapBuffers();
		m_window.pollEvents();
	}

	std::cout << "[APP] shutting down after " << m_time.frameCount() << " frames" << std::endl;
}

void Application::update(double deltaTime)
{
	if (m_input.keyPressed(GLFW_KEY_ESCAPE))
		m_window.requestClose();

	m_camera.update(m_input, deltaTime);

	// Slow spin on the second object only, so the two transforms stay visibly
	// independent while the first keeps the starter's exact pose.
	if (m_secondQuad != nullptr)
	{
		const double angle = m_time.elapsed() * 0.5;
		m_secondQuad->transform().rotation = glm::angleAxis(angle, glm::dvec3(0.0, 1.0, 0.0));
	}

	m_scene.update(deltaTime);
}

void Application::render()
{
	m_renderer.beginFrame(m_camera, m_window.aspectRatio());
	m_scene.render(m_renderer);
	m_renderer.endFrame();
}
