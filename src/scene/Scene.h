#ifndef SCENE_H
#define SCENE_H

#include <memory>
#include <string>
#include <vector>

#include "SceneObject.h"

class Renderer;

// Owns the root-level scene objects and drives update/render traversal.
// Roots are named groups (bodies, orbit_guides, spacecraft, ...), so whole
// categories can be found, toggled and traversed together.
class Scene
{
public:
	SceneObject& addObject(std::unique_ptr<SceneObject> object);

	// Returns the root group with this name, creating an empty one if needed.
	SceneObject& group(const std::string& name);
	SceneObject* findGroup(const std::string& name) const;
	const std::vector<std::unique_ptr<SceneObject>>& roots() const { return m_objects; }

	void update(double dt);
	void render(Renderer& renderer);

private:
	std::vector<std::unique_ptr<SceneObject>> m_objects;
};

#endif
