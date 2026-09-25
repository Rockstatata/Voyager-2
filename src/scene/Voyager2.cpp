#include "Voyager2.h"

#include <algorithm>
#include <cmath>

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "../core/Input.h"

namespace
{
	// Orientation whose +Z is `forward` and whose +Y is as close to world up
	// as possible: the historical probe cruises level, without arbitrary roll.
	glm::dquat orientationFromForward(const glm::dvec3& forward)
	{
		glm::dvec3 upReference(0.0, 1.0, 0.0);
		if (std::abs(glm::dot(forward, upReference)) > 0.999)
			upReference = glm::dvec3(1.0, 0.0, 0.0);
		const glm::dvec3 right = glm::normalize(glm::cross(upReference, forward));
		const glm::dvec3 up = glm::cross(forward, right);
		return glm::normalize(glm::quat_cast(glm::dmat3(right, up, forward)));
	}
}

void Voyager2::setFlightMode(FlightMode mode)
{
	if (mode == FlightMode::Manual && m_mode == FlightMode::Historical)
	{
		// Keep facing, and keep a gentle drift along it so the hand-over is
		// continuous rather than a sudden stop or a mission-speed rocket.
		const double speed = std::min(glm::length(m_velocity), 0.25);
		m_velocity = forward() * speed;
	}
	m_mode = mode;
}

void Voyager2::setHistoricalState(const glm::dvec3& position, const glm::dvec3& motionDirection,
	double renderSpeed, bool snap)
{
	transform().position = position;
	const double length = glm::length(motionDirection);
	if (length <= 1e-12)
		return;

	const glm::dvec3 direction = motionDirection / length;
	m_velocity = direction * renderSpeed;
	// Heading eases toward the motion; the whip around a planet in a few
	// hours of mission time is still a visible turn, not a snap.
	const glm::dquat target = orientationFromForward(direction);
	m_orientation = snap ? target : glm::normalize(glm::slerp(m_orientation, target, 0.2));
}

void Voyager2::update(double dt)
{
	if (m_mode == FlightMode::Manual)
		transform().position += m_velocity * dt;

	transform().rotation = m_orientation;
	SceneObject::update(dt);
}

void Voyager2::applyManualControl(const Input& input, double dt)
{
	// Rotations about the ship's own axes: multiply on the right.
	double yaw = 0.0;
	double pitch = 0.0;
	double roll = 0.0;
	if (input.keyDown(GLFW_KEY_A)) yaw += 1.0;
	if (input.keyDown(GLFW_KEY_D)) yaw -= 1.0;
	if (input.keyDown(GLFW_KEY_R)) pitch -= 1.0; // nose up
	if (input.keyDown(GLFW_KEY_F)) pitch += 1.0; // nose down
	if (input.keyDown(GLFW_KEY_Q)) roll -= 1.0;
	if (input.keyDown(GLFW_KEY_E)) roll += 1.0;

	const glm::dquat turn =
		glm::angleAxis(yaw * kTurnRateRadiansPerSecond * dt, glm::dvec3(0.0, 1.0, 0.0)) *
		glm::angleAxis(pitch * kTurnRateRadiansPerSecond * dt, glm::dvec3(1.0, 0.0, 0.0)) *
		glm::angleAxis(roll * kRollRateRadiansPerSecond * dt, glm::dvec3(0.0, 0.0, 1.0));
	m_orientation = glm::normalize(m_orientation * turn);

	double accel = kManualAccel;
	if (input.keyDown(GLFW_KEY_LEFT_SHIFT) || input.keyDown(GLFW_KEY_RIGHT_SHIFT))
		accel *= kBoostMultiplier;

	if (input.keyDown(GLFW_KEY_W))
		m_velocity += forward() * accel * dt;
	if (input.keyDown(GLFW_KEY_S))
		m_velocity -= forward() * accel * dt;
	if (input.keyDown(GLFW_KEY_SPACE))
		m_velocity += up() * accel * dt;
	if (input.keyDown(GLFW_KEY_LEFT_CONTROL))
		m_velocity -= up() * accel * dt;
	if (input.keyDown(GLFW_KEY_X))
	{
		// Braking burn: opposes the current velocity without overshooting.
		const double speed = glm::length(m_velocity);
		const double reduction = std::min(speed, accel * 2.0 * dt);
		if (speed > 1e-12)
			m_velocity -= (m_velocity / speed) * reduction;
	}

	const double speed = glm::length(m_velocity);
	if (speed > kManualMaxSpeed)
		m_velocity = (m_velocity / speed) * kManualMaxSpeed;
}
