#ifndef SOLAR_SYSTEM_H
#define SOLAR_SYSTEM_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "CelestialBody.h"
#include "../rendering/RayTraceScene.h"

// Bible section 18/52 - registry that inserts every CelestialBody into the
// scene tree (under the `bodies` group if it has no parent, a child of its
// parent body otherwise) and indexes it by id for lookup/focus/toggle. It
// also records every ring band, so the ray tracer can intersect them.
//
// Bodies must be added parent-before-child (Sun and planets before their
// moons) since a moon's parentId is resolved against what's already
// registered. SolarSystem does not own the bodies; the scene tree does.
class SolarSystem
{
public:
	explicit SolarSystem(SceneObject& root) : m_root(root) {}

	CelestialBody& addBody(CelestialBodyData data, std::shared_ptr<Mesh> mesh,
							std::shared_ptr<Material> material);

	CelestialBody* find(const std::string& id) const;
	const std::vector<CelestialBody*>& bodies() const { return m_bodies; }

	// A ring band drawn as a child of `planet`, radii in planet radii.
	struct RingBand
	{
		const CelestialBody* planet = nullptr;
		float innerRadius = 0.0f;
		float outerRadius = 0.0f;
		glm::vec3 color{ 1.0f };
		float opacity = 1.0f;
	};
	void registerRingBand(const RingBand& band) { m_ringBands.push_back(band); }
	const std::vector<RingBand>& ringBands() const { return m_ringBands; }

	// Exact spheres and annuli for this frame (world space), for the
	// ray-traced shadows and the ray-traced view.
	void buildTraceScene(RayTraceScene& scene, const glm::dvec3& sunPosition) const;

private:
	SceneObject& m_root; // the scene's `bodies` group
	std::unordered_map<std::string, CelestialBody*> m_byId;
	std::vector<CelestialBody*> m_bodies; // insertion order, non-owning
	std::vector<RingBand> m_ringBands;
};

#endif
