#include "Camera.h"

#include <algorithm>
#include <cmath>

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "../core/Input.h"

namespace
{
	double smoothStep(double t)
	{
		t = std::clamp(t, 0.0, 1.0);
		return t * t * (3.0 - 2.0 * t);
	}
}

void Camera::setYawPitch(float yawDegrees, float pitchDegrees)
{
	m_yaw = yawDegrees;
	m_pitch = glm::clamp(pitchDegrees, -kPitchLimit, kPitchLimit);
	updateBasis();
}

void Camera::lookAt(const glm::dvec3& target)
{
	const glm::dvec3 direction = target - m_position;
	if (glm::dot(direction, direction) < 1e-24)
		return;
	const glm::dvec3 unit = glm::normalize(direction);
	setYawPitch(static_cast<float>(glm::degrees(std::atan2(unit.z, unit.x))),
		static_cast<float>(glm::degrees(std::asin(glm::clamp(unit.y, -1.0, 1.0)))));
}

void Camera::updateFreeFly(const Input& input, double deltaTime, double baseSpeed, bool look)
{
	// Leaving an orbit/chase view keeps the exact current view direction.
	if (!m_freeFlyActive)
	{
		m_freeFlyActive = true;
		m_transition = 1.0;
		setYawPitch(static_cast<float>(glm::degrees(std::atan2(m_forward.z, m_forward.x))),
			static_cast<float>(glm::degrees(std::asin(glm::clamp(m_forward.y, -1.0f, 1.0f)))));
	}

	if (look)
	{
		const glm::dvec2 delta = input.mouseDelta();
		m_yaw += static_cast<float>(delta.x) * m_mouseSensitivity;
		m_pitch = glm::clamp(m_pitch - static_cast<float>(delta.y) * m_mouseSensitivity,
			-kPitchLimit, kPitchLimit);
	}
	// Arrow keys turn too, for trackpads and for anyone not holding a mouse.
	const float turn = 70.0f * static_cast<float>(deltaTime);
	if (input.keyDown(GLFW_KEY_LEFT)) m_yaw -= turn;
	if (input.keyDown(GLFW_KEY_RIGHT)) m_yaw += turn;
	if (input.keyDown(GLFW_KEY_UP)) m_pitch = glm::min(m_pitch + turn, kPitchLimit);
	if (input.keyDown(GLFW_KEY_DOWN)) m_pitch = glm::max(m_pitch - turn, -kPitchLimit);
	updateBasis();

	// Each wheel notch changes cruise speed by 30 %, over a 10^6 range.
	if (input.scrollDelta() != 0.0)
		m_speedMultiplier = std::clamp(m_speedMultiplier * std::pow(1.3, input.scrollDelta()), 1e-3, 1e3);

	double speed = baseSpeed * m_speedMultiplier;
	if (input.keyDown(GLFW_KEY_LEFT_SHIFT) || input.keyDown(GLFW_KEY_RIGHT_SHIFT))
		speed *= 6.0;
	if (input.keyDown(GLFW_KEY_LEFT_ALT) || input.keyDown(GLFW_KEY_RIGHT_ALT))
		speed *= 0.15;

	glm::dvec3 move(0.0);
	if (input.keyDown(GLFW_KEY_W)) move += glm::dvec3(m_forward);
	if (input.keyDown(GLFW_KEY_S)) move -= glm::dvec3(m_forward);
	if (input.keyDown(GLFW_KEY_D)) move += glm::dvec3(m_right);
	if (input.keyDown(GLFW_KEY_A)) move -= glm::dvec3(m_right);
	if (input.keyDown(GLFW_KEY_SPACE) || input.keyDown(GLFW_KEY_E)) move += glm::dvec3(0.0, 1.0, 0.0);
	if (input.keyDown(GLFW_KEY_LEFT_CONTROL) || input.keyDown(GLFW_KEY_Q)) move -= glm::dvec3(0.0, 1.0, 0.0);

	if (glm::dot(move, move) > 0.0)
		m_position += glm::normalize(move) * speed * deltaTime;
}

void Camera::beginOrbit(double distance, float yawDegrees, float pitchDegrees)
{
	m_orbitDistance = distance;
	m_orbitYaw = yawDegrees;
	m_orbitPitch = pitchDegrees;
	m_transition = 0.0;
	m_transitionStart = m_position;
	m_transitionStartForward = m_forward;
	m_frameInitialised = false;
}

void Camera::updateOrbit(const glm::dvec3& target, const glm::dquat& frame, double minDistance,
	double maxDistance, const Input& input, double deltaTime, bool look)
{
	m_freeFlyActive = false;
	if (look)
	{
		const glm::dvec2 delta = input.mouseDelta();
		m_orbitYaw -= static_cast<float>(delta.x) * m_mouseSensitivity;
		m_orbitPitch = glm::clamp(m_orbitPitch + static_cast<float>(delta.y) * m_mouseSensitivity,
			-85.0f, 85.0f);
	}
	const float turn = 70.0f * static_cast<float>(deltaTime);
	if (input.keyDown(GLFW_KEY_LEFT)) m_orbitYaw += turn;
	if (input.keyDown(GLFW_KEY_RIGHT)) m_orbitYaw -= turn;
	if (input.keyDown(GLFW_KEY_UP)) m_orbitPitch = glm::min(m_orbitPitch + turn, 85.0f);
	if (input.keyDown(GLFW_KEY_DOWN)) m_orbitPitch = glm::max(m_orbitPitch - turn, -85.0f);

	// Wheel zoom is multiplicative, so it is equally usable at 1.2 radii
	// (surface close-up) and at 200 radii (whole moon system).
	if (input.scrollDelta() != 0.0)
		m_orbitDistance *= std::pow(0.85, input.scrollDelta());
	m_orbitDistance = std::clamp(m_orbitDistance, minDistance, maxDistance);

	// The rig frame is smoothed so a sudden heading change (Voyager swinging
	// around Jupiter in a few hours of mission time) turns the view gently.
	if (!m_frameInitialised)
	{
		m_smoothedFrame = frame;
		m_frameInitialised = true;
	}
	else
	{
		const double blend = 1.0 - std::exp(-deltaTime * 3.0);
		m_smoothedFrame = glm::normalize(glm::slerp(m_smoothedFrame, frame, blend));
	}

	const double yaw = glm::radians(static_cast<double>(m_orbitYaw));
	const double pitch = glm::radians(static_cast<double>(m_orbitPitch));
	// Local +Z is the frame's forward, so yaw 0 places the camera behind (-Z).
	const glm::dvec3 localOffset(std::sin(yaw) * std::cos(pitch), std::sin(pitch),
		-std::cos(yaw) * std::cos(pitch));
	const glm::dvec3 frameUp = m_smoothedFrame * glm::dvec3(0.0, 1.0, 0.0);
	const glm::dvec3 desired = target + (m_smoothedFrame * localOffset) * m_orbitDistance;

	if (m_transition < 1.0)
	{
		// Fly-to: an eased path from wherever the camera was. The target is
		// tracked every frame, so the move lands exactly even on a moving body.
		m_transition = std::min(1.0, m_transition + deltaTime / kTransitionSeconds);
		const double s = smoothStep(m_transition);
		// Logarithmic distance blend: long hops start fast and settle gently.
		const glm::dvec3 fromTarget = m_transitionStart - target;
		const double startDistance = glm::length(fromTarget);
		const double finalDistance = glm::length(desired - target);
		const double distance = std::exp(glm::mix(std::log(std::max(startDistance, 1e-9)),
			std::log(std::max(finalDistance, 1e-9)), s));
		const glm::dvec3 startDirection = startDistance > 1e-12 ? fromTarget / startDistance
			: glm::normalize(desired - target);
		const glm::dvec3 finalDirection = glm::normalize(desired - target);
		glm::dvec3 direction = glm::mix(startDirection, finalDirection, s);
		if (glm::dot(direction, direction) < 1e-12)
			direction = finalDirection;
		m_position = target + glm::normalize(direction) * distance;
		const glm::vec3 lookForward = glm::normalize(glm::vec3(target - m_position));
		m_forward = glm::normalize(glm::mix(m_transitionStartForward, lookForward, static_cast<float>(
			smoothStep(m_transition * 2.0))));
	}
	else
	{
		m_position = desired;
		m_forward = glm::normalize(glm::vec3(target - m_position));
	}

	glm::vec3 upReference(frameUp);
	if (std::abs(glm::dot(upReference, m_forward)) > 0.995f)
		upReference = glm::vec3(m_smoothedFrame * glm::dvec3(0.0, 0.0, 1.0));
	m_right = glm::normalize(glm::cross(m_forward, upReference));
	m_up = glm::normalize(glm::cross(m_right, m_forward));
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

glm::mat4 Camera::viewMatrixAtOrigin() const
{
	return glm::lookAt(glm::vec3(0.0f), m_forward, m_up);
}

glm::mat4 Camera::projectionMatrix(float aspectRatio) const
{
	// The far plane is effectively unbounded; log depth (see default.frag)
	// provides the precision, so no mode has to trade near detail for range.
	return glm::perspective(glm::radians(m_fovDegrees), aspectRatio, kNearPlane, 1.0e6f);
}
