#ifndef INSTANCED_FIELD_H
#define INSTANCED_FIELD_H

#include "SceneObject.h"

// A leaf SceneObject that draws its mesh via Renderer::submitInstanced
// instead of submit — the belt/starfield-of-rocks path (bible failure mode
// F9: never one draw call per asteroid). Its own transform is unused; every
// instance's final world position/scale is already baked into the matrices
// passed to Mesh::setInstanceTransforms at construction time, since a belt
// has no single meaningful "position" of its own.
class InstancedField : public SceneObject
{
public:
	explicit InstancedField(std::string name) : SceneObject(std::move(name)) {}

	void render(Renderer& renderer) override;
};

#endif
