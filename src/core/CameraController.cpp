#include "CameraController.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

#include "Input.h"
#include "../rendering/Camera.h"
#include "../scene/MissionController.h"
#include "../scene/SolarSystem.h"
#include "../scene/Voyager2.h"

#include <GLFW/glfw3.h>

CameraController::CameraController(Camera& camera, const SolarSystem& system, const MissionController& mission)
	: m_camera(camera), m_system(system), m_mission(mission)
{
}

const CelestialBody* CameraController::focusedBody() const
{
	if (m_focusIndex < 0 || m_focusIndex >= static_cast<int>(m_system.bodies().size()))
		return nullptr;
	return m_system.bodies()[m_focusIndex];
}

double CameraController::nearestSurfaceDistance(const glm::dvec3& point) const
{
	double nearest = glm::length(point - m_mission.sunPosition());
	for (const CelestialBody* body : m_system.bodies())
	{
		const glm::dmat4 world = body->worldMatrix();
		const double radius = glm::length(glm::dvec3(world[0]));
		nearest = std::min(nearest, glm::length(point - glm::dvec3(world[3])) - radius);
	}
	if (m_voyager != nullptr)
		nearest = std::min(nearest, glm::length(point - m_voyager->transform().position) - m_voyager->boundingRadius());
	return std::max(nearest, 0.0);
}

void CameraController::update(Input& input, double deltaTime, bool pilotingVoyager, bool scripted)
{
	const bool movementKey = input.keyDown(GLFW_KEY_W) || input.keyDown(GLFW_KEY_A) ||
		input.keyDown(GLFW_KEY_S) || input.keyDown(GLFW_KEY_D) || input.keyDown(GLFW_KEY_Q) ||
		input.keyDown(GLFW_KEY_E) || input.keyDown(GLFW_KEY_SPACE) || input.keyDown(GLFW_KEY_LEFT_CONTROL);

	// Full autonomy: any fly key hands a locked view straight to free flight
	// from exactly where the camera is (piloting Voyager excepted).
	if (movementKey && !pilotingVoyager && !scripted && m_mode != Mode::FreeFly)
		m_mode = Mode::FreeFly;

	const bool look = m_mouseLookLatched || input.mouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT);
	input.setCursorCaptured(look);

	if (m_mode == Mode::FreeFly)
	{
		// Speed follows the distance to the nearest surface: centimetres per
		// second beside Voyager, hundreds of units per second between planets.
		const double baseSpeed = std::clamp(nearestSurfaceDistance(m_camera.position()) * 0.9, 0.004, 300.0);
		m_camera.updateFreeFly(input, deltaTime, baseSpeed, look);
	}
	else if (m_mode == Mode::Chase && m_voyager != nullptr)
	{
		m_camera.updateOrbit(m_voyager->transform().position, chaseFrame(),
			m_voyager->boundingRadius() * 1.3, 5000.0, input, deltaTime, look);
	}
	else if (m_mode == Mode::Inspect && m_voyager != nullptr)
	{
		// Rides in Voyager's own frame so a component stays framed as the
		// craft turns; zooms down to 0.3 of a component's size.
		const double size = m_componentIndex < 0 ? m_voyager->boundingRadius()
			: m_voyager->components()[m_componentIndex].size;
		const glm::dvec3 target = m_componentIndex < 0 ? m_voyager->transform().position
			: m_voyager->componentWorldCentre(static_cast<std::size_t>(m_componentIndex));
		m_camera.updateOrbit(target, m_voyager->orientation(), size * 0.3, 5000.0, input, deltaTime, look);
	}
	else if (const CelestialBody* focused = focusedBody(); m_mode == Mode::Focus && focused != nullptr)
	{
		const glm::dmat4 world = focused->worldMatrix();
		const double radius = glm::length(glm::dvec3(world[0]));
		m_camera.updateOrbit(glm::dvec3(world[3]), glm::dquat(1.0, 0.0, 0.0, 0.0), radius * 1.08,
			5000.0, input, deltaTime, look);
	}
}

glm::dquat CameraController::chaseFrame() const
{
	// Normally the rig rides in Voyager's own frame. During a historical
	// flyby it turns to face the planet instead, so the classic shot (the
	// spacecraft in the foreground, the world it is passing behind it) holds
	// through the whole encounter. The camera smooths the switch.
	if (m_voyager->flightMode() == Voyager2::FlightMode::Historical)
	{
		const glm::dvec3 voyagerPosition = m_voyager->transform().position;
		const CelestialBody* planet = m_mission.encounterPlanetNear(voyagerPosition, 6.0);
		// Departure: look back at Earth while Voyager is still beside it.
		const CelestialBody* earth = m_system.find("earth");
		if (planet == nullptr && earth != nullptr &&
			glm::length(earth->transform().position - voyagerPosition) < earth->transform().scale.x * 12.0)
			planet = earth;
		if (planet != nullptr)
		{
			const glm::dvec3 toPlanet = planet->transform().position - voyagerPosition;
			if (glm::length(toPlanet) > 1e-9)
				return frameLookingAlong(glm::normalize(toPlanet));
		}
	}
	return m_voyager->orientation();
}

glm::dquat CameraController::frameLookingAlong(const glm::dvec3& forward)
{
	glm::dvec3 upReference(0.0, 1.0, 0.0);
	if (std::abs(glm::dot(forward, upReference)) > 0.999)
		upReference = glm::dvec3(1.0, 0.0, 0.0);
	const glm::dvec3 right = glm::normalize(glm::cross(upReference, forward));
	const glm::dvec3 up = glm::cross(forward, right);
	return glm::normalize(glm::quat_cast(glm::dmat3(right, up, forward)));
}

void CameraController::goToOverview()
{
	const glm::dvec3 sun = m_mission.sunPosition();
	m_mode = Mode::FreeFly;
	m_camera.setPosition(sun + glm::dvec3(0.0, 62.0, 150.0));
	m_camera.lookAt(sun + glm::dvec3(0.0, 0.0, 12.0));
	std::cout << "[APP] overview camera" << std::endl;
}

void CameraController::focusBody(int index)
{
	if (index < 0 || index >= static_cast<int>(m_system.bodies().size()))
		return;
	m_focusIndex = index;
	m_mode = Mode::Focus;
	const CelestialBody* body = m_system.bodies()[index];
	const glm::dmat4 world = body->worldMatrix();
	const double radius = glm::length(glm::dvec3(world[0]));
	// Planets with moons open wide enough to show their system.
	const bool hasMoons = std::any_of(body->children().begin(), body->children().end(),
		[](const std::unique_ptr<SceneObject>& child)
		{
			return dynamic_cast<const CelestialBody*>(child.get()) != nullptr;
		});
	const double distance = radius * (body->data().type == BodyType::Star ? 4.0 : (hasMoons ? 5.5 : 3.2));
	// Open on the day side: the camera sits 40 degrees round from the
	// body-to-Sun direction (the rig's local offset is (sin yaw, -, -cos yaw)).
	const glm::dvec3 toSun = m_mission.sunPosition() - glm::dvec3(world[3]);
	float yaw = 25.0f;
	if (glm::length(glm::dvec3(toSun.x, 0.0, toSun.z)) > 1e-6)
		yaw = static_cast<float>(glm::degrees(std::atan2(toSun.x, -toSun.z))) + 40.0f;
	m_camera.beginOrbit(distance, yaw, 18.0f);
	std::cout << "[APP] focused: " << body->data().displayName << std::endl;
}

void CameraController::focusNext(int direction)
{
	const int count = static_cast<int>(m_system.bodies().size());
	if (count > 0)
		focusBody(((m_focusIndex + direction) % count + count) % count);
}

void CameraController::refocus()
{
	if (m_focusIndex >= 0)
		focusBody(m_focusIndex);
}

void CameraController::inspect(int componentIndex)
{
	if (m_voyager == nullptr)
		return;
	const int count = static_cast<int>(m_voyager->components().size());
	m_componentIndex = componentIndex >= count ? -1 : componentIndex;
	m_mode = Mode::Inspect;
	const double size = m_componentIndex < 0 ? m_voyager->boundingRadius()
		: m_voyager->components()[m_componentIndex].size;

	// Open on the lit side: blend "outward from the craft's centre" (so the
	// camera is never inside the bus or behind the dish) with "toward the
	// Sun" (so the part is not in its own shadow), in Voyager's frame.
	const glm::dquat toLocal = glm::inverse(m_voyager->orientation());
	const glm::dvec3 toSun = glm::normalize(toLocal * (m_mission.sunPosition() - m_voyager->transform().position));
	glm::dvec3 outward = m_componentIndex < 0 ? glm::dvec3(0.45, 0.35, -0.8)
		: m_voyager->components()[m_componentIndex].localCentre;
	outward = glm::length(outward) > 1e-9 ? glm::normalize(outward) : glm::dvec3(0.0, 0.3, -1.0);
	const glm::dvec3 view = glm::normalize(outward + toSun * 0.8 + glm::dvec3(0.0, 0.25, 0.0));
	// The orbit rig's local offset is (sin yaw cos pitch, sin pitch, -cos yaw cos pitch).
	const float yaw = static_cast<float>(glm::degrees(std::atan2(view.x, -view.z)));
	const float pitch = static_cast<float>(glm::degrees(std::asin(glm::clamp(view.y, -0.95, 0.95))));
	m_camera.beginOrbit(size * (m_componentIndex < 0 ? 1.5 : 1.9), yaw, pitch);
	std::cout << "[APP] inspecting: "
			  << (m_componentIndex < 0 ? std::string("Voyager 2") : m_voyager->components()[m_componentIndex].name)
			  << std::endl;
}

void CameraController::inspectNext(int direction)
{
	if (m_voyager == nullptr)
		return;
	// Cycle through -1 (whole craft), 0 .. count-1.
	const int slots = static_cast<int>(m_voyager->components().size()) + 1;
	const int slot = ((m_componentIndex + 1 + direction) % slots + slots) % slots;
	inspect(slot - 1);
}

void CameraController::enterChase()
{
	m_mode = Mode::Chase;
	// A three-quarter view from behind and above; wheel zooms, RMB orbits.
	m_camera.beginOrbit(m_voyager != nullptr ? m_voyager->boundingRadius() * 4.0 : 0.1, 20.0f, 10.0f);
}
