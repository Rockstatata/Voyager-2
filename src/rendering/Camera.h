#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>

class Input;

// Free-fly camera (bible section 37, "free camera" mode).
//
// Position is double precision because the solar system will eventually place
// the camera billions of kilometres from the origin; the view matrix is
// produced in float only at upload time. The camera never writes simulation
// state — moving it must not change any body's physical position.
class Camera
{
public:
	void setPosition(const glm::dvec3& position) { m_position = position; }
	void setYawPitch(float yawDegrees, float pitchDegrees);
	void setPerspective(float fovDegrees, float nearPlane, float farPlane);

	// Free-fly update driven entirely through Input: WASD strafe, Space/Ctrl
	// up-down, Shift boost, right mouse button to look. Uses unscaled dt so a
	// simulation speed multiplier can never affect camera feel.
	void update(const Input& input, double deltaTime);

	glm::dvec3 position() const { return m_position; }
	glm::vec3 forward() const { return m_forward; }
	glm::vec3 right() const { return m_right; }
	glm::vec3 up() const { return m_up; }

	glm::mat4 viewMatrix() const;
	glm::mat4 projectionMatrix(float aspectRatio) const;

	float moveSpeed() const { return m_moveSpeed; }
	void setMoveSpeed(float unitsPerSecond) { m_moveSpeed = unitsPerSecond; }

private:
	void updateBasis();

	glm::dvec3 m_position{ 0.0, 0.0, 2.0 };

	// -90 degrees of yaw points the camera down -Z, which reproduces the
	// starter project's hardcoded translate(0, 0, -2) view matrix exactly.
	float m_yaw = -90.0f;
	float m_pitch = 0.0f;

	glm::vec3 m_forward{ 0.0f, 0.0f, -1.0f };
	glm::vec3 m_right{ 1.0f, 0.0f, 0.0f };
	glm::vec3 m_up{ 0.0f, 1.0f, 0.0f };

	float m_fovDegrees = 45.0f;
	float m_nearPlane = 0.1f;
	float m_farPlane = 100.0f;

	float m_moveSpeed = 2.0f;        // world units per second
	float m_boostMultiplier = 4.0f;
	float m_mouseSensitivity = 0.1f; // degrees per pixel
	static constexpr float kPitchLimit = 89.0f;
};

#endif
