#ifndef VOYAGER_2_H
#define VOYAGER_2_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

#include "SceneObject.h"

class Input;

// Bible section 31/32 — the spacecraft. Its body meshes (bus/dish/booms) are
// built and attached as children by Application; this class only owns
// flight state (bible's "Voyager Dual-Mode Architecture", section 32-35).
//
// Historical mode interpolates offline NASA/JPL Horizons samples. It is a
// real heliocentric path. Application synchronizes the planets to the same
// selected historical date; manual mode pauses that date while piloting stays
// an independent inertial experience.
// Manual mode is inertial: turning (yaw) changes only where the ship *faces*,
// not its velocity — matching how a real thruster-driven spacecraft behaves
// (no atmosphere to turn against), and distinguishing this from arcade flight.
class Voyager2 : public SceneObject
{
public:
	enum class FlightMode { Historical, Manual };

	explicit Voyager2(std::string name = "voyager2") : SceneObject(std::move(name)) {}

	void update(double dt) override;

	void setFlightMode(FlightMode mode);
	FlightMode flightMode() const { return m_mode; }

	// Application calls this only while flightMode() == Manual, after reading
	// Input itself — Voyager2 never polls GLFW directly (bible section 36).
	void applyManualControl(const Input& input, double dt);
	void setHistoricalPath(std::vector<glm::dvec3> renderPositions,
		std::vector<double> julianDates, double playbackSeconds);
	void setHistoricalProgress(double normalizedProgress);
	void setHistoricalJulianDate(double julianDate);
	void setHistoricalTimeScale(double timeScale) { m_historicalTimeScale = timeScale; }
	double historicalJulianDate() const;
	const glm::dvec3& velocity() const { return m_velocity; }

	glm::vec3 headingForward() const;

private:
	FlightMode m_mode = FlightMode::Historical;
	float m_yawDegrees = 90.0f;              // 90 deg => heading (1,0,0), facing +X
	glm::dvec3 m_velocity{ 0.6, 0.0, 0.0 };  // render units/second
	std::vector<glm::dvec3> m_historicalPath;
	std::vector<double> m_historicalJulianDates;
	double m_historicalProgress = 0.0;
	double m_historicalPlaybackSeconds = 120.0;
	double m_historicalStartJulianDate = 0.0;
	double m_historicalEndJulianDate = 0.0;
	double m_currentHistoricalJulianDate = 0.0;
	double m_historicalTimeScale = 1.0;

	void updateHistoricalPosition();

	static constexpr float kYawRateDegreesPerSecond = 60.0f;
	static constexpr double kManualAccel = 1.2;
	static constexpr double kManualMaxSpeed = 1.6;
};

#endif
