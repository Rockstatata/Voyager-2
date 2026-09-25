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

CelestialBody* SolarSystem::find(const std::string& id) const
{
	auto it = m_byId.find(id);
	return it != m_byId.end() ? it->second : nullptr;
}
