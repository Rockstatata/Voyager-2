#ifndef SOLAR_SYSTEM_H
#define SOLAR_SYSTEM_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "CelestialBody.h"
#include "Scene.h"

// Bible section 18/52 — registry that owns every CelestialBody by inserting
// it into the Scene tree (root if it has no parent, a child of its parent
// body otherwise) and indexing it by id for lookup/focus/toggle.
//
// Bodies must be added parent-before-child (Sun and planets before their
// moons) since a moon's parentId is resolved against what's already
// registered; SolarSystem does not own the bodies itself; Scene does.
class SolarSystem
{
public:
	explicit SolarSystem(Scene& scene) : m_scene(scene) {}

	CelestialBody& addBody(CelestialBodyData data, std::shared_ptr<Mesh> mesh,
							std::shared_ptr<Material> material);

	CelestialBody* find(const std::string& id) const;
	const std::vector<CelestialBody*>& bodies() const { return m_bodies; }

private:
	Scene& m_scene;
	std::unordered_map<std::string, CelestialBody*> m_byId;
	std::vector<CelestialBody*> m_bodies; // insertion order, non-owning
};

#endif
