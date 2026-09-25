#ifndef LIGHTING_CONTROLLER_H
#define LIGHTING_CONTROLLER_H

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "../rendering/Lighting.h"

class Camera;
class Input;

// The scene's light rig and its keyboard switches (docs/objects/lighting.md):
//   light 0  Sun          point light at the Sun's centre; optional
//                         inverse-square falloff with compressed distance
//   light 1  Headlamp     spotlight on the camera, pointing where it looks
//   light 2  Fill         dim directional light from ecliptic north
// plus the global shading technique and the lighting master switch.
class LightingController
{
public:
	// K master switch, F3 technique, F5 headlamp, F6 fill, F7 Sun falloff,
	// F8 lighting maps, F4 ray-traced shadows (off / hard / soft).
	void handleKeys(const Input& input);

	// Rebuilds the per-frame state. `surfaceDistance` scales the headlamp's
	// reach so it is useful beside Voyager and beside Jupiter alike.
	LightingState build(const glm::dvec3& sunPosition, const Camera& camera, double surfaceDistance) const;

	void setTechnique(ShadingTechnique technique) { m_technique = technique; }
	void setHeadlamp(bool on) { m_headlamp = on; }
	void setFill(bool on) { m_fill = on; }
	void setSunFalloff(bool on) { m_sunFalloff = on; }
	void setSurfaceMaps(bool on) { m_surfaceMaps = on; }
	void setShadows(ShadowMode mode) { m_shadows = mode; }
	ShadingTechnique technique() const { return m_technique; }
	std::vector<std::string> statusLines() const;

private:
	bool m_enabled = true;
	ShadingTechnique m_technique = ShadingTechnique::BlinnPhong;
	bool m_headlamp = false;
	bool m_fill = false;
	bool m_sunFalloff = false;
	bool m_surfaceMaps = true;
	ShadowMode m_shadows = ShadowMode::Soft;
};

#endif
