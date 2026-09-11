#ifndef SCENE_H
#define SCENE_H

#include <memory>
#include <vector>

#include "SceneObject.h"

class Renderer;

// Owns the root-level scene objects and drives update/render traversal.
class Scene
{
public:
	SceneObject& addObject(std::unique_ptr<SceneObject> object);

	void update(double dt);
	void render(Renderer& renderer);

private:
	std::vector<std::unique_ptr<SceneObject>> m_objects;
};

#endif
