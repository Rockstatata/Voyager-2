#ifndef SCENE_OBJECT_H
#define SCENE_OBJECT_H

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

#include "Transform.h"
#include "../rendering/Mesh.h"

class Renderer;

// A node in the scene graph. Owns a local transform and an optional mesh,
// plus child objects whose world transforms derive from this one.
//
// The mesh is held by shared_ptr because geometry is shared, not duplicated:
// every planet will draw the same sphere (bible section 41). Uploading one
// sphere per body is the failure mode this prevents.
class SceneObject
{
public:
	SceneObject() = default;
	explicit SceneObject(std::string name) : m_name(std::move(name)) {}
	virtual ~SceneObject() = default;

	virtual void update(double dt);
	virtual void render(Renderer& renderer);

	Transform& transform() { return m_transform; }
	const Transform& transform() const { return m_transform; }

	void setMesh(std::shared_ptr<Mesh> mesh);
	const std::shared_ptr<Mesh>& mesh() const { return m_mesh; }

	const std::string& name() const { return m_name; }
	void setName(std::string name) { m_name = std::move(name); }

	void setVisible(bool visible) { m_visible = visible; }
	bool visible() const { return m_visible; }

	SceneObject& addChild(std::unique_ptr<SceneObject> child);
	const std::vector<std::unique_ptr<SceneObject>>& children() const { return m_children; }

	// worldTransform = parentWorldTransform x localTransform
	glm::dmat4 worldMatrix() const;

protected:
	std::string m_name;
	Transform m_transform;
	SceneObject* m_parent = nullptr; // non-owning; owned via parent's m_children
	std::vector<std::unique_ptr<SceneObject>> m_children;
	std::shared_ptr<Mesh> m_mesh;    // shared geometry, never per-object copies
	bool m_visible = true;
};

#endif
