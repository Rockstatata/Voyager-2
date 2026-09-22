#include "Window.h"

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

void Window::errorCallback(int code, const char* description)
{
	std::cout << "[GLFW] error " << code << ": " << (description != nullptr ? description : "") << std::endl;
}

void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);

	Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (self != nullptr)
	{
		self->m_width = width;
		self->m_height = height;
	}
}

bool Window::create(int width, int height, const std::string& title)
{
	glfwSetErrorCallback(&Window::errorCallback);

	if (!glfwInit())
	{
		std::cout << "[GLFW] glfwInit failed" << std::endl;
		return false;
	}
	m_glfwInitialised = true;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	if (m_window == nullptr)
	{
		std::cout << "[GLFW] failed to create window" << std::endl;
		glfwTerminate();
		m_glfwInitialised = false;
		return false;
	}

	glfwMakeContextCurrent(m_window);

	// GLAD must load only once a context is current.
	if (!gladLoadGL())
	{
		std::cout << "[OPENGL] gladLoadGL failed" << std::endl;
		glfwDestroyWindow(m_window);
		m_window = nullptr;
		glfwTerminate();
		m_glfwInitialised = false;
		return false;
	}

	// The framebuffer can differ from the requested size on scaled displays,
	// so the viewport is driven from the real framebuffer size.
	glfwGetFramebufferSize(m_window, &m_width, &m_height);
	glViewport(0, 0, m_width, m_height);

	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, &Window::framebufferSizeCallback);

	std::cout << "[OPENGL] " << (const char*)glGetString(GL_VERSION)
			  << " | " << (const char*)glGetString(GL_RENDERER) << std::endl;

	return true;
}

Window::~Window()
{
	if (m_window != nullptr)
		glfwDestroyWindow(m_window);
	if (m_glfwInitialised)
		glfwTerminate();
}

bool Window::shouldClose() const
{
	return m_window == nullptr || glfwWindowShouldClose(m_window) != 0;
}

void Window::requestClose()
{
	if (m_window != nullptr)
		glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

void Window::pollEvents()
{
	glfwPollEvents();
}

void Window::swapBuffers()
{
	if (m_window != nullptr)
		glfwSwapBuffers(m_window);
}

void Window::setTitle(const std::string& title)
{
	if (m_window != nullptr)
		glfwSetWindowTitle(m_window, title.c_str());
}

float Window::aspectRatio() const
{
	if (m_width <= 0 || m_height <= 0)
		return 1.0f;
	return (float)m_width / (float)m_height;
}
