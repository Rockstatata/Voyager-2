#include "SceneObject.h"

#include "../rendering/Renderer.h"

void SceneObject::update(double dt)
{
	for (auto& child : m_children)
		child->update(dt);
}

void SceneObject::render(Renderer& renderer)
{
	if (!m_visible)
		return;

	if (m_mesh != nullptr)
	{
		static const Material kDefaultMaterial;
		const Material& material = m_material != nullptr ? *m_material : kDefaultMaterial;
		renderer.submit(*m_mesh, material, glm::mat4(worldMatrix()));
	}

	for (auto& child : m_children)
		child->render(renderer);
}

void SceneObject::setMesh(std::shared_ptr<Mesh> mesh)
{
	m_mesh = std::move(mesh);
}

void SceneObject::setMaterial(std::shared_ptr<Material> material)
{
	m_material = std::move(material);
}

SceneObject& SceneObject::addChild(std::unique_ptr<SceneObject> child)
{
	child->m_parent = this;
	m_children.push_back(std::move(child));
	return *m_children.back();
}

glm::dmat4 SceneObject::worldMatrix() const
{
	const glm::dmat4 local = m_transform.localMatrix();
	if (m_parent != nullptr)
		return m_parent->worldMatrix() * local;
	return local;
}
