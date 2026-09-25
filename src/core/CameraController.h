#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Camera;
class CelestialBody;
class Input;
class MissionController;
class SolarSystem;
class Voyager2;

// Decides WHICH rig drives the Camera and WHAT it looks at (bible sections
// 37 and 40): free flight, the chase rig on Voyager, Focus on a body, or
// Inspect: a close-up orbit around Voyager or one of its named components.
// Camera itself only knows how to move; this class owns modes, selection,
// framing and speed policy. It never edits simulation state.
class CameraController
{
public:
	enum class Mode { FreeFly, Chase, Focus, Inspect };

	CameraController(Camera& camera, const SolarSystem& system, const MissionController& mission);

	void setVoyager(const Voyager2* voyager) { m_voyager = voyager; }

	// `pilotingVoyager`: the chase camera shares the keyboard with manual
	// flight, so fly keys must not break out of it. `scripted`: a capture
	// tour owns the camera and keys are ignored.
	void update(Input& input, double deltaTime, bool pilotingVoyager, bool scripted);

	void goToOverview();
	void focusBody(int index);
	void focusNext(int direction);
	void refocus();
	void enterChase();
	void enterFreeFly() { m_mode = Mode::FreeFly; }
	void toggleMouseLook() { m_mouseLookLatched = !m_mouseLookLatched; }

	// Inspect mode: -1 = the whole spacecraft, otherwise a component index.
	void inspect(int componentIndex);
	void inspectNext(int direction);
	int inspectedComponent() const { return m_componentIndex; }

	Mode mode() const { return m_mode; }
	int focusIndex() const { return m_focusIndex; }
	const CelestialBody* focusedBody() const;
	bool mouseLookLatched() const { return m_mouseLookLatched; }

	// Distance from `point` to the nearest body or spacecraft surface.
	double nearestSurfaceDistance(const glm::dvec3& point) const;

	static glm::dquat frameLookingAlong(const glm::dvec3& forward);

private:
	glm::dquat chaseFrame() const;

	Camera& m_camera;
	const SolarSystem& m_system;
	const MissionController& m_mission;
	const Voyager2* m_voyager = nullptr;
	Mode m_mode = Mode::Chase;
	int m_focusIndex = -1;
	int m_componentIndex = -1;
	bool m_mouseLookLatched = false;
};

#endif
