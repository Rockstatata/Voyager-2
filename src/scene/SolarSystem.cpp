#include "SolarSystem.h"

#include <iostream>

CelestialBody& SolarSystem::addBody(CelestialBodyData data, std::shared_ptr<Mesh> mesh,
									 std::shared_ptr<Material> material)
{
	const std::string id = data.id;
	const std::string parentId = data.parentId;

	auto body = std::make_unique<CelestialBody>(std::move(data), std::move(mesh), std::move(material));
	CelestialBody* raw = body.get();

	if (parentId.empty())
	{
		m_root.addChild(std::move(body));
	}
	else
	{
		auto it = m_byId.find(parentId);
		if (it == m_byId.end())
		{
			std::cout << "[SOLAR] parent '" << parentId << "' not yet registered for body '"
					  << id << "' — added as a scene root instead" << std::endl;
			m_root.addChild(std::move(body));
		}
		else
		{
			it->second->addChild(std::move(body));
		}
	}

	m_byId[id] = raw;
	m_bodies.push_back(raw);
	return *raw;
}

void SolarSystem::buildTraceScene(RayTraceScene& scene, const glm::dvec3& sunPosition) const
{
	scene.spheres.clear();
	scene.rings.clear();
	scene.sunPosition = sunPosition;

	int layer = 0;
	for (const CelestialBody* body : m_bodies)
	{
		const glm::dmat4 surface = body->surfaceMatrix();
		const double radius = glm::length(glm::dvec3(surface[0]));
		TraceSphere sphere;
		sphere.center = glm::dvec3(surface[3]);
		sphere.radius = radius;
		// Columns of the surface matrix are the scaled local axes; dividing by
		// the radius leaves the pure rotation, whose transpose is its inverse.
		sphere.worldToLocal = glm::transpose(glm::mat3(glm::dmat3(surface) / radius));
		sphere.textureLayer = layer++;
		sphere.emissive = body->data().type == BodyType::Star;
		if (const Material* material = body->material().get())
		{
			sphere.specularStrength = material->specularStrength;
			sphere.specularPower = material->specularPower;
		}
		if (body->data().materialId == "ocean")
			sphere.reflectivity = 0.12f;
		else if (body->data().materialId == "ice")
			sphere.reflectivity = 0.06f;
		scene.spheres.push_back(sphere);
	}

	for (const RingBand& band : m_ringBands)
	{
		const glm::dmat4 world = band.planet->worldMatrix();
		const double planetRadius = glm::length(glm::dvec3(world[0]));
		TraceRing ring;
		ring.center = glm::dvec3(world[3]);
		ring.normal = glm::vec3(glm::normalize(glm::dvec3(world[1])));
		ring.innerRadius = band.innerRadius * planetRadius;
		ring.outerRadius = band.outerRadius * planetRadius;
		ring.color = band.color;
		ring.opacity = band.opacity;
		scene.rings.push_back(ring);
	}
}

CelestialBody* SolarSystem::find(const std::string& id) const
{
	auto it = m_byId.find(id);
	return it != m_byId.end() ? it->second : nullptr;
}
