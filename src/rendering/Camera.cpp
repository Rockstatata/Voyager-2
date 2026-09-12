#include "Camera.h"

#include <cmath>

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "../core/Input.h"

void Camera::setYawPitch(float yawDegrees, float pitchDegrees)
{
	m_yaw = yawDegrees;
	m_pitch = glm::clamp(pitchDegrees, -kPitchLimit, kPitchLimit);
	updateBasis();
}

void Camera::setPerspective(float fovDegrees, float nearPlane, float farPlane)
{
	m_fovDegrees = fovDegrees;
	m_nearPlane = nearPlane;
	m_farPlane = farPlane;
}

void Camera::update(const Input& input, double deltaTime)
{
	// Look: only while the right mouse button is held, so a stray mouse move
	// cannot silently rotate the view during the lab demo.
	if (input.mouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
	{
		const glm::dvec2 delta = input.mouseDelta();
		m_yaw += (float)delta.x * m_mouseSensitivity;
		m_pitch -= (float)delta.y * m_mouseSensitivity; // screen Y grows downward
		m_pitch = glm::clamp(m_pitch, -kPitchLimit, kPitchLimit);
	}

	updateBasis();

	float speed = m_moveSpeed;
	if (input.keyDown(GLFW_KEY_LEFT_SHIFT) || input.keyDown(GLFW_KEY_RIGHT_SHIFT))
		speed *= m_boostMultiplier;

	glm::dvec3 move(0.0);
	if (input.keyDown(GLFW_KEY_W)) move += glm::dvec3(m_forward);
	if (input.keyDown(GLFW_KEY_S)) move -= glm::dvec3(m_forward);
	if (input.keyDown(GLFW_KEY_D)) move += glm::dvec3(m_right);
	if (input.keyDown(GLFW_KEY_A)) move -= glm::dvec3(m_right);
	if (input.keyDown(GLFW_KEY_SPACE)) move += glm::dvec3(0.0, 1.0, 0.0);
	if (input.keyDown(GLFW_KEY_LEFT_CONTROL)) move -= glm::dvec3(0.0, 1.0, 0.0);

	// Normalising keeps diagonal movement from being faster than straight.
	if (glm::dot(move, move) > 0.0)
		m_position += glm::normalize(move) * (double)speed * deltaTime;
}

void Camera::updateBasis()
{
	const float yaw = glm::radians(m_yaw);
	const float pitch = glm::radians(m_pitch);

	glm::vec3 forward;
	forward.x = std::cos(pitch) * std::cos(yaw);
	forward.y = std::sin(pitch);
	forward.z = std::cos(pitch) * std::sin(yaw);

	m_forward = glm::normalize(forward);
	m_right = glm::normalize(glm::cross(m_forward, glm::vec3(0.0f, 1.0f, 0.0f)));
	m_up = glm::normalize(glm::cross(m_right, m_forward));
}

glm::mat4 Camera::viewMatrix() const
{
	// Position is kept in double and narrowed only here. Once the world grows
	// past float precision, the floating-origin phase replaces this by
	// subtracting the eye position from every model matrix instead.
	const glm::vec3 eye(m_position);
	return glm::lookAt(eye, eye + m_forward, m_up);
}

glm::mat4 Camera::projectionMatrix(float aspectRatio) const
{
	return glm::perspective(glm::radians(m_fovDegrees), aspectRatio, m_nearPlane, m_farPlane);
}
