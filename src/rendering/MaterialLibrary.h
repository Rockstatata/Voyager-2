#ifndef MATERIAL_LIBRARY_H
#define MATERIAL_LIBRARY_H

#include <memory>
#include <string>

#include <glm/glm.hpp>

#include "Material.h"

// Builds every Material in the scene from a small set of named presets, so
// how a surface class responds to light is decided in one place rather than
// scattered through scene construction. Surface presets (catalog `material`
// column): emissive, rocky, ocean, cloud, gas, ice.
class MaterialLibrary
{
public:
	// Credited albedo texture plus the preset's lighting response. A missing
	// file falls back to flat grey with texturing disabled.
	static std::shared_ptr<Material> surface(const std::string& texturePath, const std::string& preset);

	static std::shared_ptr<Material> flat(const glm::vec3& color, ShadingModel shading = ShadingModel::Unlit,
		float opacity = 1.0f);

	// Additive halo; `falloff` is the facing exponent (see lighting.md).
	static std::shared_ptr<Material> glow(const glm::vec3& color, float opacity, float falloff);

	// Painted/foil spacecraft hardware.
	static std::shared_ptr<Material> spacecraft(const glm::vec3& color);
};

#endif
