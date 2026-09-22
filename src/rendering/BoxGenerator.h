#ifndef BOX_GENERATOR_H
#define BOX_GENERATOR_H

#include "MeshData.h"

// Builds a rectangular box centred at the origin. Each face owns four
// vertices: positions along a hard edge are shared spatially, but not in the
// vertex buffer, because the adjoining faces need different normals and UVs.
class BoxGenerator
{
public:
	static MeshData generate(float width = 1.0f, float height = 1.0f, float depth = 1.0f);
};

#endif
