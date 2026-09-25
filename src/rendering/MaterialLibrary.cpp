#include "MaterialLibrary.h"

#include <vector>

#include "SurfaceMaps.h"

std::shared_ptr<Material> MaterialLibrary::surface(const std::string& texturePath, const std::string& preset)
{
	auto material = std::make_shared<Material>();
	material->baseColor = glm::vec3(0.6f);

	int width = 0;
	int height = 0;
	std::vector<unsigned char> albedo;
	if (Texture2D::decodeFile(texturePath, width, height, albedo))
	{
		auto texture = std::make_shared<Texture2D>();
		if (texture->uploadRgba(width, height, albedo, texturePath))
		{
			material->albedoTexture = std::move(texture);
			material->baseColor = glm::vec3(1.0f);
		}
	}
	const bool hasAlbedo = material->albedoTexture != nullptr;

	// Lighting maps derived from the same photograph (SurfaceMaps): bump
	// relief for solid surfaces, an ocean glint mask for Earth.
	auto uploadMap = [&](const std::vector<unsigned char>& pixels, const std::string& suffix)
	{
		auto map = std::make_shared<Texture2D>();
		map->uploadRgba(width, height, pixels, texturePath + suffix);
		return map;
	};
	if (hasAlbedo && (preset == "rocky" || preset == "ice" || preset == "ocean"))
	{
		const float relief = preset == "rocky" ? 2.0f : (preset == "ice" ? 1.6f : 1.2f);
		material->normalTexture = uploadMap(SurfaceMaps::normalMapFromAlbedo(albedo, width, height, relief),
			" [normal]");
	}
	if (hasAlbedo && preset == "ocean")
		material->specularTexture = uploadMap(SurfaceMaps::oceanSpecularMap(albedo, width, height), " [specular]");

	// How each surface class answers the Sun: specular strength is the
	// fraction of light mirrored, power the tightness of the highlight.
	if (preset == "emissive")
	{
		material->shading = ShadingModel::Unlit; // the light source itself
	}
	else if (preset == "ocean")
	{
		material->specularStrength = 0.55f; // sun glint on water (masked to oceans)
		material->specularPower = 48.0f;
	}
	else if (preset == "ice")
	{
		material->specularStrength = 0.22f; // bright, fairly sharp
		material->specularPower = 36.0f;
	}
	else if (preset == "gas")
	{
		material->specularStrength = 0.06f; // soft cloud-top sheen
		material->specularPower = 10.0f;
	}
	else if (preset == "cloud")
	{
		material->specularStrength = 0.10f; // thick haze: broad and dull
		material->specularPower = 6.0f;
	}
	else
	{
		material->specularStrength = 0.03f; // rocky regolith: almost matte
		material->specularPower = 8.0f;
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
