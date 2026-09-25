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

	// Lighting maps (texture units 1 and 2), derived from the albedo by
	// SurfaceMaps: a tangent-space normal map that bends the lighting normal
	// for craters and ridges, and a specular map that scales the highlight
	// per texel (water glints, land does not).
	std::shared_ptr<Texture2D> normalTexture;
	std::shared_ptr<Texture2D> specularTexture;
	float normalStrength = 1.0f;

	// Texture-atlas window: the mesh's own 0..1 UVs are mapped to
	// uv * (z, w) + (x, y), so one atlas image serves many parts
	// (Voyager's NASA hardware photographs). Identity = (0, 0, 1, 1).
	glm::vec4 uvTransform{ 0.0f, 0.0f, 1.0f, 1.0f };
};

#endif
