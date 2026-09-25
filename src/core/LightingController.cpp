#include "LightingController.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "Input.h"
#include "../rendering/Camera.h"

#include <GLFW/glfw3.h>

namespace
{
	const char* onOff(bool value)
	{
		return value ? "ON" : "OFF";
	}
}

void LightingController::handleKeys(const Input& input)
{
	if (input.keyPressed(GLFW_KEY_K))
	{
		m_enabled = !m_enabled;
		std::cout << "[APP] lighting " << (m_enabled ? "on" : "off") << std::endl;
	}
	if (input.keyPressed(GLFW_KEY_F3))
	{
		const bool backward = input.keyDown(GLFW_KEY_LEFT_SHIFT) || input.keyDown(GLFW_KEY_RIGHT_SHIFT);
		const int count = 5;
		const int next = (static_cast<int>(m_technique) + (backward ? count - 1 : 1)) % count;
		m_technique = static_cast<ShadingTechnique>(next);
		std::cout << "[APP] shading technique: " << shadingTechniqueName(m_technique) << std::endl;
	}
	if (input.keyPressed(GLFW_KEY_F5))
		m_headlamp = !m_headlamp;
	if (input.keyPressed(GLFW_KEY_F6))
		m_fill = !m_fill;
	if (input.keyPressed(GLFW_KEY_F7))
		m_sunFalloff = !m_sunFalloff;
	if (input.keyPressed(GLFW_KEY_F8))
		m_surfaceMaps = !m_surfaceMaps;
	if (input.keyPressed(GLFW_KEY_F4))
		m_shadows = static_cast<ShadowMode>((static_cast<int>(m_shadows) + 1) % 3);
}

LightingState LightingController::build(const glm::dvec3& sunPosition, const Camera& camera,
	double surfaceDistance) const
{
	LightingState state;
	state.enabled = m_enabled;
	state.technique = m_technique;
	state.ambient = 0.07f;
	state.surfaceMaps = m_surfaceMaps;
	state.shadows = m_shadows;

	// Sun: a point light. With falloff on, 1 / (0.3 + 0.7 (d / 20)^2), which
	// is 1.0 at Earth's 20-unit orbit and ~0.03 at Neptune (compressed
	// distances make the real inverse-square law gentler than 1/900).
	Light& sun = state.lights[0];
	sun.type = LightType::Point;
	sun.enabled = true;
	sun.position = sunPosition;
	sun.color = glm::vec3(1.0f, 0.98f, 0.94f);
	sun.intensity = 1.0f;
	sun.attenuation = m_sunFalloff ? glm::vec3(0.3f, 0.0f, 0.7f / 400.0f) : glm::vec3(1.0f, 0.0f, 0.0f);

	// Headlamp: a spotlight at the eye along the view direction, 12-18
	// degree soft cone. Quadratic falloff scaled to the nearest surface.
	Light& headlamp = state.lights[1];
	headlamp.type = LightType::Spot;
	headlamp.enabled = m_headlamp;
	headlamp.position = camera.position();
	headlamp.direction = camera.forward();
	headlamp.color = glm::vec3(1.0f, 0.93f, 0.80f);
	headlamp.intensity = 1.4f;
	const float reach = static_cast<float>(std::max(surfaceDistance, 0.01) * 3.0);
	headlamp.attenuation = glm::vec3(1.0f, 0.0f, 1.0f / (reach * reach));
	headlamp.innerCutoffCos = std::cos(glm::radians(12.0f));
	headlamp.outerCutoffCos = std::cos(glm::radians(18.0f));

	// Fill: directional light from ecliptic north, cool and dim, so night
	// sides and the far faces of Voyager keep readable form.
	Light& fill = state.lights[2];
	fill.type = LightType::Directional;
	fill.enabled = m_fill;
	fill.direction = glm::normalize(glm::vec3(0.25f, -1.0f, 0.15f));
	fill.color = glm::vec3(0.55f, 0.65f, 0.85f);
	fill.intensity = 0.35f;

	return state;
}

std::vector<std::string> LightingController::statusLines() const
{
	std::vector<std::string> lines;
	if (!m_enabled)
	{
		lines.push_back("LIGHTING OFF (K)");
		return lines;
	}
	const char* shadowNames[] = { "OFF", "HARD", "SOFT" };
	lines.push_back(std::string("SHADING ") + shadingTechniqueName(m_technique) + " (F3)   RAY-TRACED SHADOWS " +
		shadowNames[static_cast<int>(m_shadows)] + " (F4)");
	lines.push_back(std::string("LIGHTS  SUN") + (m_sunFalloff ? " 1/D2" : "") +
		"  HEADLAMP " + onOff(m_headlamp) + "  FILL " + onOff(m_fill) + "  MAPS " + onOff(m_surfaceMaps));
	return lines;
}
