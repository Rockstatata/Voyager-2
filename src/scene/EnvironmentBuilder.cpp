#include "EnvironmentBuilder.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Comet.h"
#include "InstancedField.h"
#include "MissionEphemeris.h"
#include "ScaleManager.h"
#include "SceneObject.h"
#include "../rendering/CircleGenerator.h"
#include "../rendering/MaterialLibrary.h"
#include "../rendering/Mesh.h"
#include "../rendering/StarfieldGenerator.h"
#include "../rendering/UvSphereGenerator.h"

namespace
{
	constexpr double kOrbitGuideEpochJulianDate = 2447000.5; // 1987-07-15, mid-mission

	std::shared_ptr<Mesh> polylineMesh(const std::vector<glm::dvec3>& points, const glm::dvec3& origin,
		PrimitiveMode mode)
	{
		MeshData data;
		data.vertices.reserve(points.size());
		data.indices.reserve(points.size());
		for (std::size_t i = 0; i < points.size(); ++i)
		{
			Vertex vertex;
			vertex.position = glm::vec3(points[i] - origin);
			vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
			vertex.texCoord = glm::vec2(static_cast<float>(i) / static_cast<float>(points.size()), 0.0f);
			data.vertices.push_back(vertex);
			data.indices.push_back(static_cast<std::uint32_t>(i));
		}
		return std::make_shared<Mesh>(data, mode);
	}

	// Point in a flat annulus (a belt), Y-jittered for thickness, with an
	// independent per-axis scale and random spin so each rock is irregular.
	std::vector<glm::mat4> generateBeltMatrices(unsigned int count, float innerRadius, float outerRadius,
		float heightJitter, float minScale, float maxScale, const glm::dvec3& center, unsigned int seed)
	{
		std::mt19937 rng(seed);
		std::uniform_real_distribution<float> unit(0.0f, 1.0f);
		std::vector<glm::mat4> matrices;
		matrices.reserve(count);
		for (unsigned int i = 0; i < count; ++i)
		{
			const float theta = glm::two_pi<float>() * unit(rng);
			const float radius = innerRadius + (outerRadius - innerRadius) * std::sqrt(unit(rng));
			const float y = (unit(rng) * 2.0f - 1.0f) * heightJitter;
			const glm::vec3 position(radius * std::cos(theta), y, radius * std::sin(theta));
			const glm::vec3 scale(
				minScale + (maxScale - minScale) * unit(rng),
				minScale + (maxScale - minScale) * unit(rng),
				minScale + (maxScale - minScale) * unit(rng));
			const float spin = glm::two_pi<float>() * unit(rng);

			glm::mat4 m(1.0f);
			m = glm::translate(m, glm::vec3(center) + position);
			m = glm::rotate(m, spin, glm::normalize(glm::vec3(unit(rng) - 0.5f, 1.0f, unit(rng) - 0.5f)));
			m = glm::scale(m, scale);
			matrices.push_back(m);
		}
		return matrices;
	}

	// Uniform point in a spherical shell (Oort cloud), radius sampled by volume.
	std::vector<glm::mat4> generateShellMatrices(unsigned int count, float innerRadius, float outerRadius,
		float minScale, float maxScale, const glm::dvec3& center, unsigned int seed)
	{
		std::mt19937 rng(seed);
		std::uniform_real_distribution<float> unit(0.0f, 1.0f);
		std::vector<glm::mat4> matrices;
		matrices.reserve(count);
		const float innerCubed = innerRadius * innerRadius * innerRadius;
		const float outerCubed = outerRadius * outerRadius * outerRadius;
		for (unsigned int i = 0; i < count; ++i)
		{
			const float theta = glm::two_pi<float>() * unit(rng);
			const float cosPhi = 2.0f * unit(rng) - 1.0f;
			const float sinPhi = std::sqrt(1.0f - cosPhi * cosPhi);
			const float radius = std::cbrt(innerCubed + (outerCubed - innerCubed) * unit(rng));
			const glm::vec3 position(radius * sinPhi * std::cos(theta), radius * cosPhi,
				radius * sinPhi * std::sin(theta));
			const glm::vec3 scale(
				minScale + (maxScale - minScale) * unit(rng),
				minScale + (maxScale - minScale) * unit(rng),
				minScale + (maxScale - minScale) * unit(rng));

			glm::mat4 m(1.0f);
			m = glm::translate(m, glm::vec3(center) + position);
			m = glm::scale(m, scale);
			matrices.push_back(m);
		}
		return matrices;
	}
}

std::vector<std::unique_ptr<SceneObject>> EnvironmentBuilder::buildStarLayers()
{
	// Three brightness layers give many faint stars and few bright ones.
	struct StarLayer { unsigned int count; unsigned int seed; glm::vec3 color; };
	const std::array<StarLayer, 3> layers = {{
		{ 5200, 11u, glm::vec3(0.34f, 0.35f, 0.40f) },
		{ 1800, 23u, glm::vec3(0.66f, 0.66f, 0.72f) },
		{ 380, 37u, glm::vec3(1.0f, 0.97f, 0.92f) },
	}};

	std::vector<std::unique_ptr<SceneObject>> result;
	for (const StarLayer& layer : layers)
	{
		auto stars = std::make_unique<SceneObject>("starfield");
		stars->setMesh(std::make_shared<Mesh>(StarfieldGenerator::generate(layer.count, 5000.0f, layer.seed),
			PrimitiveMode::Points));
		stars->setMaterial(MaterialLibrary::flat(layer.color));
		result.push_back(std::move(stars));
	}
	return result;
}

void EnvironmentBuilder::buildOrbitGuides(SceneObject& group, const MissionEphemeris& ephemeris,
	const glm::dvec3& sunPosition)
{
	// The osculating two-body ellipse through each planet's Horizons state,
	// mapped with the same distance law: every planet rides its own line.
	for (const MissionEphemeris::Planet& planet : ephemeris.planets())
	{
		const std::vector<glm::dvec3> guide = ephemeris.orbitGuide(planet, kOrbitGuideEpochJulianDate, 360);
		if (guide.empty())
			continue;
		const bool dwarf = planet.id == "pluto";
		auto orbit = std::make_unique<SceneObject>(planet.id + "_orbit");
		orbit->setMesh(polylineMesh(guide, sunPosition, PrimitiveMode::LineLoop));
		orbit->setMaterial(MaterialLibrary::flat(dwarf ? glm::vec3(0.24f, 0.22f, 0.30f) : glm::vec3(0.22f, 0.30f, 0.40f)));
		orbit->transform().position = sunPosition;
		group.addChild(std::move(orbit));
	}
}

void EnvironmentBuilder::buildHeliosphere(SceneObject& group, const glm::dvec3& sunPosition)
{
	// Three great circles per boundary, at Voyager 2's own crossing distances:
	// termination shock 84 AU (2007), heliopause 119 AU (2018-11-05).
	const ScaleManager scaleManager;
	auto circleMesh = std::make_shared<Mesh>(CircleGenerator::generate(), PrimitiveMode::LineLoop);
	auto addBoundary = [&](const std::string& name, double radius, const glm::vec3& color)
	{
		const std::array<glm::dquat, 3> rotations = {{
			glm::dquat(1.0, 0.0, 0.0, 0.0),
			glm::angleAxis(glm::radians(90.0), glm::dvec3(1.0, 0.0, 0.0)),
			glm::angleAxis(glm::radians(90.0), glm::dvec3(0.0, 0.0, 1.0)),
		}};
		for (std::size_t plane = 0; plane < rotations.size(); ++plane)
		{
			auto loop = std::make_unique<SceneObject>(name + "_plane_" + std::to_string(plane));
			loop->setMesh(circleMesh);
			loop->setMaterial(MaterialLibrary::flat(color));
			loop->transform().position = sunPosition;
			loop->transform().rotation = rotations[plane];
			loop->transform().scale = glm::dvec3(radius);
			group.addChild(std::move(loop));
		}
	};
	addBoundary("termination_shock", scaleManager.distanceAuToRenderUnits(84.0), glm::vec3(0.09f, 0.17f, 0.24f));
	addBoundary("heliopause", scaleManager.distanceAuToRenderUnits(119.0), glm::vec3(0.18f, 0.13f, 0.28f));
}

void EnvironmentBuilder::buildSmallBodyFields(SceneObject& group, const glm::dvec3& sunPosition)
{
	// One instanced draw call per field (bible F9). Each field needs its own
	// Mesh because the instance buffer lives on the mesh.
	const ScaleManager scaleManager;
	const MeshData rockData = UvSphereGenerator::generate(6, 8);
	auto addField = [&](const std::string& name, std::vector<glm::mat4> matrices, const glm::vec3& color)
	{
		auto mesh = std::make_shared<Mesh>(rockData);
		mesh->setInstanceTransforms(matrices);
		auto field = std::make_unique<InstancedField>(name);
		field->setMesh(mesh);
		field->setMaterial(MaterialLibrary::flat(color, ShadingModel::Lit));
		group.addChild(std::move(field));
		std::cout << "[SCENE] " << name << ": " << matrices.size() << " instances, 1 draw call" << std::endl;
	};
	auto radiusAt = [&scaleManager](double au)
	{
		return static_cast<float>(scaleManager.distanceAuToRenderUnits(au));
	};

	addField("asteroid_belt",
		generateBeltMatrices(4000, radiusAt(2.1), radiusAt(3.3), 0.6f, 0.012f, 0.05f, sunPosition, 101),
		glm::vec3(0.62f, 0.57f, 0.50f));
	addField("kuiper_belt",
		generateBeltMatrices(3000, radiusAt(30.0), radiusAt(50.0), 3.0f, 0.06f, 0.20f, sunPosition, 202),
		glm::vec3(0.62f, 0.66f, 0.74f));
	addField("oort_cloud",
		generateShellMatrices(1500, radiusAt(2000.0), radiusAt(5000.0), 1.5f, 4.0f, sunPosition, 303),
		glm::vec3(0.70f, 0.76f, 0.84f));
}

void EnvironmentBuilder::buildComet(SceneObject& group, const glm::dvec3& sunPosition,
	const std::shared_ptr<Mesh>& sphere)
{
	group.addChild(std::make_unique<Comet>(sunPosition, sphere));
}
