#ifndef MATERIAL_H
#define MATERIAL_H

#include <memory>

#include <glm/glm.hpp>

#include "Texture2D.h"

// How a surface responds to the Sun's point light (bible section 47).
//   Unlit        - emissive or abstract guide geometry: Sun, lines, stars.
//   Lit          - diffuse + optional specular from the Sun.
//   LitTwoSided  - thin sheets (rings) lit from whichever face sees the Sun.
//   Glow         - additive halo drawn after opaque geometry; alpha falls
//                  off toward the shell silhouette as facing^specularPower.
enum class ShadingModel { Unlit = 0, Lit = 1, LitTwoSided = 2, Glow = 3 };

// One material seam shared by every renderable object. Shader uniforms are
// derived only from these fields in Renderer, never from object types.
struct Material
{
	glm::vec3 baseColor{ 1.0f };
	std::shared_ptr<Texture2D> albedoTexture;
	ShadingModel shading = ShadingModel::Lit;
	float specularStrength = 0.0f;
	float specularPower = 32.0f;
	float opacity = 1.0f;
};

#endif
