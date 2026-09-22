#ifndef MATERIAL_H
#define MATERIAL_H

#include <memory>

#include <glm/glm.hpp>

#include "Texture2D.h"

// Phase-2 material seam. Later instructor shaders can extend this without
// changing the sphere generator or celestial object transforms.
struct Material
{
	glm::vec3 baseColor{ 1.0f };
	std::shared_ptr<Texture2D> albedoTexture;
};

#endif
