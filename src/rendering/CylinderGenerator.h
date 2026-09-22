#ifndef CYLINDER_GENERATOR_H
#define CYLINDER_GENERATOR_H

#include "MeshData.h"

// Pure CPU geometry module (no GL context needed), same shape as
// UvSphereGenerator. Produces a cylinder/cone/frustum: two radii let one
// generator serve a straight cylinder (radiusBottom == radiusTop, the
// Voyager bus/rods), a cone (radiusTop == 0, comet tail), or a frustum.
// The lateral surface duplicates its seam column exactly like the UV
// sphere, for the same reason: u=0 and u=1 are the same edge in space but
// need different U values.
class CylinderGenerator
{
public:
	static MeshData generate(float radiusBottom, float radiusTop, float height,
							 unsigned int radialSegments = 16, bool capBottom = true,
							 bool capTop = true);
};

#endif
