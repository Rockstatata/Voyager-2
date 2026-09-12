#include "Input.h"

#include <GLFW/glfw3.h>

void Input::attach(GLFWwindow* window)
{
	m_window = window;
	m_keys.fill(false);
	m_previousKeys.fill(false);
	m_mouseButtons.fill(false);
	m_previousMouseButtons.fill(false);
	m_hasMouseSample = false;
	m_mouseDelta = glm::dvec2(0.0);
}

void Input::update()
{
	if (m_window == nullptr)
		return;

	m_previousKeys = m_keys;
	m_previousMouseButtons = m_mouseButtons;

	for (int key = 0; key <= GLFW_KEY_LAST && key < kKeyCount; ++key)
		m_keys[key] = glfwGetKey(m_window, key) == GLFW_PRESS;

	for (int button = 0; button < kMouseButtonCount; ++button)
		m_mouseButtons[button] = glfwGetMouseButton(m_window, button) == GLFW_PRESS;

	double x = 0.0;
	double y = 0.0;
	glfwGetCursorPos(m_window, &x, &y);
	const glm::dvec2 position(x, y);

	// First sample has no predecessor, so it must not produce a delta.
	m_mouseDelta = m_hasMouseSample ? (position - m_mousePosition) : glm::dvec2(0.0);
	m_mousePosition = position;
	m_hasMouseSample = true;
}

bool Input::keyDown(int key) const
{
	return validKey(key) && m_keys[key];
}

bool Input::keyPressed(int key) const
{
	return validKey(key) && m_keys[key] && !m_previousKeys[key];
}

bool Input::keyReleased(int key) const
{
	return validKey(key) && !m_keys[key] && m_previousKeys[key];
}

bool Input::mouseButtonDown(int button) const
{
	return validButton(button) && m_mouseButtons[button];
}

bool Input::mouseButtonPressed(int button) const
{
	return validButton(button) && m_mouseButtons[button] && !m_previousMouseButtons[button];
}

void Input::setCursorCaptured(bool captured)
{
	if (m_window == nullptr || captured == m_cursorCaptured)
		return;

	m_cursorCaptured = captured;
	glfwSetInputMode(m_window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

	// Drop the stale position so the next update() reports a zero delta.
	m_hasMouseSample = false;
	m_mouseDelta = glm::dvec2(0.0);
}
