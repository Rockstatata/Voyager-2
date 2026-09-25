#include "MaterialLibrary.h"

std::shared_ptr<Material> MaterialLibrary::surface(const std::string& texturePath, const std::string& preset)
{
	auto material = std::make_shared<Material>();
	material->baseColor = glm::vec3(0.6f);

	auto texture = std::make_shared<Texture2D>();
	if (texture->loadFromFile(texturePath))
	{
		material->albedoTexture = std::move(texture);
		material->baseColor = glm::vec3(1.0f);
	}

	if (preset == "emissive")
	{
		material->shading = ShadingModel::Unlit; // the light source itself
	}
	else if (preset == "ocean")
	{
		material->specularStrength = 0.18f;
		material->specularPower = 32.0f;
	}
	else if (preset == "gas" || preset == "cloud")
	{
		material->specularStrength = 0.05f;
		material->specularPower = 12.0f;
	}
	return material;
}

std::shared_ptr<Material> MaterialLibrary::flat(const glm::vec3& color, ShadingModel shading, float opacity)
{
	auto material = std::make_shared<Material>();
	material->baseColor = color;
	material->shading = shading;
	material->opacity = opacity;
	return material;
}

std::shared_ptr<Material> MaterialLibrary::glow(const glm::vec3& color, float opacity, float falloff)
{
	auto material = flat(color, ShadingModel::Glow, opacity);
	material->specularPower = falloff;
	return material;
}

std::shared_ptr<Material> MaterialLibrary::spacecraft(const glm::vec3& color)
{
	auto material = std::make_shared<Material>();
	material->baseColor = color;
	material->specularStrength = 0.35f;
	material->specularPower = 24.0f;
	return material;
}
