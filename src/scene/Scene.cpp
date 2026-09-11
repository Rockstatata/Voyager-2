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
