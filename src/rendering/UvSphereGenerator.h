#ifndef UV_SPHERE_GENERATOR_H
#define UV_SPHERE_GENERATOR_H

#include "MeshData.h"

// Pure CPU geometry module: no OpenGL context is required to generate data.
class UvSphereGenerator
{
public:
	static MeshData generate(unsigned int latitudeSegments = 32,
							 unsigned int longitudeSegments = 64);
};

#endif
