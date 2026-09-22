#ifndef STARFIELD_GENERATOR_H
#define STARFIELD_GENERATOR_H

#include "MeshData.h"

// Pure CPU geometry module. Produces `count` points uniformly distributed
// on a sphere shell of the given radius, meant to be drawn with
// PrimitiveMode::Points (Mesh.h) — one Mesh, one draw call for the entire
// background starfield (the same F9 reasoning as belt instancing, but here
// a single mesh is enough since GL_POINTS needs no per-instance transform:
// every star's final position is just a vertex).
class StarfieldGenerator
{
public:
	static MeshData generate(unsigned int count, float radius, unsigned int seed = 1);
};

#endif
