#ifndef SCENE_OBJECT_H
#define SCENE_OBJECT_H

#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "Transform.h"
#include "../rendering/Mesh.h"

class Renderer;

// A node in the scene graph. Owns a local transform and optional mesh,
// plus child objects whose world transforms derive from this one.
class SceneObject
{
public:
	virtual ~SceneObject() = default;

	virtual void update(double dt);
	virtual void render(Renderer& renderer);

	Transform& transform() { return m_transform; }
	const Transform& transform() const { return m_transform; }

	void setMesh(std::unique_ptr<Mesh> mesh);

	SceneObject& addChild(std::unique_ptr<SceneObject> child);

	// worldTransform = parentWorldTransform x localTransform
	glm::dmat4 worldMatrix() const;

protected:
	Transform m_transform;
	SceneObject* m_parent = nullptr; // non-owning; owned via parent's m_children
	std::vector<std::unique_ptr<SceneObject>> m_children;
	std::unique_ptr<Mesh> m_mesh;
};

#endif
