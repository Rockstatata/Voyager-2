#include "SolarSystemBuilder.h"

#include <iostream>
#include <map>

#include <glm/gtc/constants.hpp>

#include "ScaleManager.h"
#include "SolarSystem.h"
#include "../rendering/MaterialLibrary.h"
#include "../rendering/Mesh.h"
#include "../rendering/RingGenerator.h"

namespace
{
	void addSunGlow(CelestialBody& sun, const std::shared_ptr<Mesh>& sphere)
	{
		// Two additive shells: a tight bright corona and a wide faint halo.
		// Children inherit the Sun's scale, so 1.0 means one solar radius.
		auto corona = std::make_unique<SceneObject>("sun_corona");
		corona->setMesh(sphere);
		corona->setMaterial(MaterialLibrary::glow(glm::vec3(1.0f, 0.80f, 0.45f), 0.9f, 1.5f));
		corona->transform().scale = glm::dvec3(1.25);
		sun.addChild(std::move(corona));

		auto halo = std::make_unique<SceneObject>("sun_halo");
		halo->setMesh(sphere);
		halo->setMaterial(MaterialLibrary::glow(glm::vec3(1.0f, 0.60f, 0.25f), 0.55f, 3.0f));
		halo->transform().scale = glm::dvec3(2.6);
		sun.addChild(std::move(halo));
	}
}

void SolarSystemBuilder::build(SolarSystem& system, const std::vector<CelestialBodyData>& bodies,
	const std::vector<RingBandData>& rings, const std::shared_ptr<Mesh>& sphere, const glm::dvec3& sunPosition)
{
	const ScaleManager scaleManager;
	std::map<std::string, int> moonsPerParent;
	int texturesLoaded = 0;

	for (const CelestialBodyData& data : bodies)
	{
		auto material = MaterialLibrary::surface(data.texturePath, data.materialId);
		if (material->albedoTexture != nullptr)
			++texturesLoaded;

		if (data.type == BodyType::Star)
		{
			CelestialBody& sun = system.addBody(data, sphere, material);
			sun.transform().position = sunPosition;
			sun.transform().scale = glm::dvec3(scaleManager.sunRadiusToRenderUnits());
			addSunGlow(sun, sphere);
			continue;
		}

		if (data.parentId.empty())
		{
			// Planets are positioned every frame from the dated ephemeris.
			CelestialBody& body = system.addBody(data, sphere, material);
			body.transform().scale = glm::dvec3(scaleManager.radiusToRenderUnits(data.radiusKm));
			continue;
		}

		// A moon's local position and scale compose through the parent's
		// world matrix, which already holds the parent's scale, so both are
		// expressed in parent radii (scale-manager.md).
		const CelestialBody* parent = system.find(data.parentId);
		const double parentRenderRadius = parent != nullptr ? parent->transform().scale.x : 1.0;
		const double parentRadiusKm = parent != nullptr ? parent->data().radiusKm : 1.0;
		const int siblingIndex = moonsPerParent[data.parentId]++;
		const double orbitWorld = scaleManager.moonOrbitDistanceToRenderUnits(
			data.semiMajorAxisKm, parentRadiusKm, parentRenderRadius);
		// Golden-angle spacing spreads the starting phases of siblings.
		const double initialAngle = siblingIndex * 2.39996322972865332;
		const double angularVelocity = glm::two_pi<double>() * kMoonDaysPerSecond / data.orbitalPeriodDays;

		CelestialBody& body = system.addBody(data, sphere, material);
		body.transform().scale = glm::dvec3(scaleManager.radiusToRenderUnits(data.radiusKm) / parentRenderRadius);
		body.setOrbit(orbitWorld / parentRenderRadius, angularVelocity, initialAngle, glm::dvec3(0.0),
			data.eccentricity);
	}

	std::cout << "[SCENE] " << system.bodies().size() << " bodies share 1 sphere mesh ("
			  << sphere->vertexCount() << " vertices, " << sphere->indexCount() / 3 << " triangles), "
			  << texturesLoaded << " real texture maps loaded" << std::endl;

	// Each named band is its own annulus in parent-radius units, so it
	// inherits the planet's scale and tilt; gaps between rows are real gaps.
	std::map<std::string, int> bandsPerPlanet;
	for (const RingBandData& band : rings)
	{
		CelestialBody* planet = system.find(band.planetId);
		if (planet == nullptr)
			continue;
		const int index = bandsPerPlanet[band.planetId]++;
		auto ring = std::make_unique<SceneObject>(band.planetId + "_ring_" + std::to_string(index));
		ring->setMesh(std::make_shared<Mesh>(RingGenerator::generate(band.innerRadius, band.outerRadius, 128)));
		ring->setMaterial(MaterialLibrary::flat(band.color, ShadingModel::LitTwoSided, band.opacity));
		planet->addChild(std::move(ring));
	}
	std::cout << "[SCENE] ring systems attached:";
	for (const auto& [planetId, count] : bandsPerPlanet)
		std::cout << ' ' << planetId << " (" << count << ")";
	std::cout << std::endl;
}
