#include "Voyager2.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include <GLFW/glfw3.h>

#include "../core/Input.h"

namespace
{
	glm::dvec3 headingFromYaw(float yawDegrees)
	{
		const float yawRadians = glm::radians(yawDegrees);
		return glm::dvec3(std::sin(yawRadians), 0.0, std::cos(yawRadians));
	}
}

void Voyager2::setFlightMode(FlightMode mode)
{
	if (mode == FlightMode::Manual && m_mode == FlightMode::Historical)
	{
		// Snap facing to match current velocity so the ship doesn't visibly
		// "pop" to face a different way the instant manual control begins.
		if (glm::length(m_velocity) > 0.0001)
		{
			const glm::dvec3 heading = glm::normalize(m_velocity);
			m_yawDegrees = glm::degrees(static_cast<float>(std::atan2(heading.x, heading.z)));
		}
	}
	m_mode = mode;
}

void Voyager2::update(double dt)
{
	if (m_mode == FlightMode::Historical && m_historicalPath.size() >= 2)
	{
		const double historicalDt = dt * m_historicalTimeScale;
		const glm::dvec3 oldPosition = transform().position;
		const double missionDays = m_historicalEndJulianDate - m_historicalStartJulianDate;
		m_currentHistoricalJulianDate += historicalDt * missionDays / m_historicalPlaybackSeconds;
		if (m_currentHistoricalJulianDate > m_historicalEndJulianDate)
		{
			// The mission is a timeline, not a looping animation. Holding the
			// final date prevents every body and the probe from teleporting back
			// to launch after the Neptune/interstellar section finishes.
			m_currentHistoricalJulianDate = m_historicalEndJulianDate;
		}
		m_historicalProgress = (m_currentHistoricalJulianDate - m_historicalStartJulianDate) / missionDays;
		updateHistoricalPosition();
		const glm::dvec3 travel = transform().position - oldPosition;
		if (glm::length(travel) > 1e-8 && historicalDt > 1e-9)
		{
			m_velocity = travel / historicalDt;
			const glm::dvec3 heading = glm::normalize(travel);
			m_yawDegrees = glm::degrees(static_cast<float>(std::atan2(heading.x, heading.z)));
		}
	}
	else
	{
		transform().position += m_velocity * dt;
	}

	transform().rotation = glm::angleAxis(glm::radians(static_cast<double>(m_yawDegrees)),
										   glm::dvec3(0.0, 1.0, 0.0));

	SceneObject::update(dt);
}

void Voyager2::setHistoricalPath(std::vector<glm::dvec3> renderPositions,
	std::vector<double> julianDates, double playbackSeconds)
{
	if (renderPositions.size() != julianDates.size() || renderPositions.size() < 2)
		return;

	m_historicalPath = std::move(renderPositions);
	m_historicalJulianDates = std::move(julianDates);
	m_historicalPlaybackSeconds = std::max(playbackSeconds, 1.0);
	m_historicalStartJulianDate = m_historicalJulianDates.front();
	m_historicalEndJulianDate = m_historicalJulianDates.back();
	m_currentHistoricalJulianDate = m_historicalStartJulianDate;
	m_historicalProgress = 0.0;
	transform().position = m_historicalPath.front();
}

double Voyager2::historicalJulianDate() const
{
	return m_currentHistoricalJulianDate;
}

void Voyager2::setHistoricalProgress(double normalizedProgress)
{
	if (m_historicalPath.size() < 2 || m_historicalJulianDates.size() != m_historicalPath.size())
		return;
	setHistoricalJulianDate(glm::mix(m_historicalStartJulianDate,
		m_historicalEndJulianDate, glm::clamp(normalizedProgress, 0.0, 1.0)));
}

void Voyager2::setHistoricalJulianDate(double julianDate)
{
	if (m_historicalPath.size() < 2 || m_historicalJulianDates.size() != m_historicalPath.size())
		return;

	m_currentHistoricalJulianDate = glm::clamp(julianDate,
		m_historicalStartJulianDate, m_historicalEndJulianDate);
	m_historicalProgress = (m_currentHistoricalJulianDate - m_historicalStartJulianDate) /
		(m_historicalEndJulianDate - m_historicalStartJulianDate);
	updateHistoricalPosition();
}

void Voyager2::updateHistoricalPosition()
{
	if (m_historicalPath.size() < 2 || m_historicalJulianDates.size() != m_historicalPath.size())
		return;

	const auto upper = std::upper_bound(m_historicalJulianDates.begin(), m_historicalJulianDates.end(),
		m_currentHistoricalJulianDate);
	if (upper == m_historicalJulianDates.begin())
	{
		transform().position = m_historicalPath.front();
		return;
	}
	if (upper == m_historicalJulianDates.end())
	{
		transform().position = m_historicalPath.back();
		return;
	}

	const std::size_t second = static_cast<std::size_t>(std::distance(m_historicalJulianDates.begin(), upper));
	const std::size_t first = second - 1;
	const double fraction = (m_currentHistoricalJulianDate - m_historicalJulianDates[first]) /
		(m_historicalJulianDates[second] - m_historicalJulianDates[first]);
	transform().position = glm::mix(m_historicalPath[first], m_historicalPath[second], fraction);
}

void Voyager2::applyManualControl(const Input& input, double dt)
{
	if (input.keyDown(GLFW_KEY_A) || input.keyDown(GLFW_KEY_LEFT))
		m_yawDegrees += kYawRateDegreesPerSecond * static_cast<float>(dt);
	if (input.keyDown(GLFW_KEY_D) || input.keyDown(GLFW_KEY_RIGHT))
		m_yawDegrees -= kYawRateDegreesPerSecond * static_cast<float>(dt);

	const glm::dvec3 heading = headingFromYaw(m_yawDegrees);
	if (input.keyDown(GLFW_KEY_W))
		m_velocity += heading * kManualAccel * dt;
	if (input.keyDown(GLFW_KEY_S))
		m_velocity -= heading * kManualAccel * dt;
	if (input.keyDown(GLFW_KEY_SPACE))
		m_velocity.y += kManualAccel * dt;
	if (input.keyDown(GLFW_KEY_LEFT_CONTROL))
		m_velocity.y -= kManualAccel * dt;

	const double speed = glm::length(m_velocity);
	if (speed > kManualMaxSpeed)
		m_velocity = (m_velocity / speed) * kManualMaxSpeed;
}

glm::vec3 Voyager2::headingForward() const
{
	return glm::vec3(headingFromYaw(m_yawDegrees));
}
