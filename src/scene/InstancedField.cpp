#include "InstancedField.h"

#include "../rendering/Renderer.h"

void InstancedField::render(Renderer& renderer)
{
	if (!visible() || mesh() == nullptr)
		return;

	static const Material kDefaultMaterial;
	const Material& mat = material() != nullptr ? *material() : kDefaultMaterial;
	renderer.submitInstanced(*mesh(), mat);
}
