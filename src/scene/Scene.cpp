#include "Scene.h"

SceneObject& Scene::addObject(std::unique_ptr<SceneObject> object)
{
	m_objects.push_back(std::move(object));
	return *m_objects.back();
}

void Scene::update(double dt)
{
	for (auto& object : m_objects)
		object->update(dt);
}

void Scene::render(Renderer& renderer)
{
	for (auto& object : m_objects)
		object->render(renderer);
}

SceneObject& Scene::group(const std::string& name)
{
	if (SceneObject* existing = findGroup(name))
		return *existing;
	return addObject(std::make_unique<SceneObject>(name));
}

SceneObject* Scene::findGroup(const std::string& name) const
{
	for (const auto& object : m_objects)
	{
		if (object->name() == name)
			return object.get();
	}
	return nullptr;
}
