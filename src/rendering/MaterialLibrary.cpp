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

MaterialLibrary::Atlas MaterialLibrary::loadAtlas(const std::string& path, float relief)
{
	Atlas atlas;
	std::vector<unsigned char> pixels;
	if (!Texture2D::decodeFile(path, atlas.width, atlas.height, pixels))
		return atlas;
	atlas.albedo = std::make_shared<Texture2D>();
	atlas.albedo->uploadRgba(atlas.width, atlas.height, pixels, path);
	atlas.normals = std::make_shared<Texture2D>();
	atlas.normals->uploadRgba(atlas.width, atlas.height,
		SurfaceMaps::normalMapFromAlbedo(pixels, atlas.width, atlas.height, relief), path + " [normal]");
	return atlas;
}

std::shared_ptr<Material> MaterialLibrary::spacecraftTextured(const Atlas& atlas, const glm::vec4& pixelRect,
	const glm::vec3& tint, float specular, float power)
{
	auto material = spacecraft(tint, specular, power);
	if (atlas.albedo == nullptr || atlas.width <= 0 || atlas.height <= 0)
		return material; // falls back to the flat tint
	material->albedoTexture = atlas.albedo;
	material->normalTexture = atlas.normals;
	material->normalStrength = 0.8f;
	// Pixel rectangle -> UV window. Images load bottom-up (v = 0 is the
	// bottom row), so the top pixel edge becomes the larger v.
	const float u0 = pixelRect.x / atlas.width;
	const float u1 = pixelRect.z / atlas.width;
	const float v0 = 1.0f - pixelRect.w / atlas.height;
	const float v1 = 1.0f - pixelRect.y / atlas.height;
	material->uvTransform = glm::vec4(u0, v0, u1 - u0, v1 - v0);
	return material;
}

std::shared_ptr<Material> MaterialLibrary::spacecraft(const glm::vec3& color, float specular, float power)
{
	auto material = std::make_shared<Material>();
	material->baseColor = color;
	material->specularStrength = specular;
	material->specularPower = power;
	return material;
}
