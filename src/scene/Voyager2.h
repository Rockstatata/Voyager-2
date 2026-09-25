#ifndef VOYAGER_2_H
#define VOYAGER_2_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "SceneObject.h"

class Input;

// Bible sections 31-35: one spacecraft, two flight modes, one model.
//
// Historical: Application samples the mission ephemeris at the simulation
// clock's date and hands the render position here each frame
// (setHistoricalState). Voyager2 never owns the date, so the planets and the
// probe can never disagree about "when" it is (bible R2).
//
// Manual: a six-degree-of-freedom inertial spacecraft. Orientation is a
// quaternion (bible F6: no Euler-angle drift or gimbal lock); yaw, pitch and
// roll rotate about the ship's OWN axes. Thrust changes velocity, never
// facing — no atmosphere to turn against — and X fires a braking burn.
//
// Local axes: +Z forward (flight direction, away from the dish), +Y up,
// +X right.
class Voyager2 : public SceneObject
{
public:
	enum class FlightMode { Historical, Manual };

	explicit Voyager2(std::string name = "voyager2") : SceneObject(std::move(name)) {}

	void update(double dt) override;

	void setFlightMode(FlightMode mode);
	FlightMode flightMode() const { return m_mode; }

	// Application calls this only while flightMode() == Manual and the chase
	// camera is active, after reading Input itself (bible section 36).
	void applyManualControl(const Input& input, double dt);

	// Historical placement for this frame: position, direction of motion and
	// render-space speed (units per real second). The heading eases toward the
	// motion unless `snap` (a bookmark jump) asks for it immediately.
	void setHistoricalState(const glm::dvec3& position, const glm::dvec3& motionDirection,
		double renderSpeed, bool snap);

	const glm::dvec3& velocity() const { return m_velocity; }
	const glm::dquat& orientation() const { return m_orientation; }
	glm::dvec3 forward() const { return m_orientation * glm::dvec3(0.0, 0.0, 1.0); }
	glm::dvec3 up() const { return m_orientation * glm::dvec3(0.0, 1.0, 0.0); }
	// Axis-aligned radius enclosing the whole model (booms included).
	double boundingRadius() const { return m_boundingRadius; }
	void setBoundingRadius(double radius) { m_boundingRadius = radius; }

private:
	FlightMode m_mode = FlightMode::Historical;
	glm::dquat m_orientation{ 1.0, 0.0, 0.0, 0.0 };
	glm::dvec3 m_velocity{ 0.0 }; // render units per second
	double m_boundingRadius = 0.03;

	static constexpr double kTurnRateRadiansPerSecond = 1.1;
	static constexpr double kRollRateRadiansPerSecond = 1.6;
	static constexpr double kManualAccel = 1.5;
	static constexpr double kBoostMultiplier = 8.0;
	static constexpr double kManualMaxSpeed = 12.0;
};

#endif
