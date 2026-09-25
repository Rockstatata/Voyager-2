#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Input;

// One camera, three rigs (bible section 37):
//   free fly  - WASD/QE/Space/Ctrl, mouse look, wheel = speed; the viewer can
//               go anywhere and inspect any object at any distance;
//   orbit     - locked on a body (Focus): drag to orbit, wheel to zoom from
//               the surface out to a whole moon system;
//   chase     - the same orbit rig expressed in Voyager's own frame, so
//               "behind the spacecraft" follows its pitch and roll.
//
// Position is double precision; the view matrix is built at the origin and
// Renderer subtracts position from every model matrix (floating origin).
// The camera never writes simulation state.
class Camera
{
public:
	void setPosition(const glm::dvec3& position) { m_position = position; }
	void setYawPitch(float yawDegrees, float pitchDegrees);
	void lookAt(const glm::dvec3& target);
	void setFieldOfView(float fovDegrees) { m_fovDegrees = fovDegrees; }

	// Free fly. `baseSpeed` (units/s) is chosen by Application from the
	// distance to the nearest surface, so flying feels the same whether the
	// camera is skimming Io or crossing the Kuiper belt. The wheel and
	// Shift/Alt multiply it; `look` rotates the view from mouse motion.
	void updateFreeFly(const Input& input, double deltaTime, double baseSpeed, bool look);

	// Starts a smooth transition into an orbit around a new target.
	void beginOrbit(double distance, float yawDegrees, float pitchDegrees);

	// Orbit rig. `frame` orients the rig (identity = world axes; Voyager's
	// rotation for the chase view). Wheel zooms between min/max distance.
	void updateOrbit(const glm::dvec3& target, const glm::dquat& frame, double minDistance,
		double maxDistance, const Input& input, double deltaTime, bool look);

	glm::dvec3 position() const { return m_position; }
	glm::vec3 forward() const { return m_forward; }
	glm::vec3 right() const { return m_right; }
	glm::vec3 up() const { return m_up; }
	float fieldOfView() const { return m_fovDegrees; }
	double speedMultiplier() const { return m_speedMultiplier; }
	double orbitDistance() const { return m_orbitDistance; }

	// Camera-relative view (eye at the origin) for the floating-origin path.
	glm::mat4 viewMatrixAtOrigin() const;
	glm::mat4 projectionMatrix(float aspectRatio) const;

	static constexpr float kNearPlane = 1.0e-6f;

private:
	void updateBasis();

	glm::dvec3 m_position{ 0.0, 0.0, 2.0 };
	float m_yaw = -90.0f;
	float m_pitch = 0.0f;
	glm::vec3 m_forward{ 0.0f, 0.0f, -1.0f };
	glm::vec3 m_right{ 1.0f, 0.0f, 0.0f };
	glm::vec3 m_up{ 0.0f, 1.0f, 0.0f };
	float m_fovDegrees = 50.0f;

	double m_speedMultiplier = 1.0;
	bool m_freeFlyActive = true; // false while an orbit/chase rig owns the view
	float m_mouseSensitivity = 0.12f; // degrees per pixel

	// Orbit rig state.
	float m_orbitYaw = 0.0f;
	float m_orbitPitch = 15.0f;
	double m_orbitDistance = 1.0;
	glm::dquat m_smoothedFrame{ 1.0, 0.0, 0.0, 0.0 };
	bool m_frameInitialised = false;
	double m_transition = 1.0;           // 0 -> 1 while flying into a new orbit
	glm::dvec3 m_transitionStart{ 0.0 }; // camera position when it began
	glm::vec3 m_transitionStartForward{ 0.0f, 0.0f, -1.0f };

	static constexpr float kPitchLimit = 89.0f;
	static constexpr double kTransitionSeconds = 1.6;
};

#endif
