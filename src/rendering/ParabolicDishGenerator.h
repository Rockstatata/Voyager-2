#ifndef PARABOLIC_DISH_GENERATOR_H
#define PARABOLIC_DISH_GENERATOR_H

#include "MeshData.h"

// Generates a shallow, double-sided paraboloid whose symmetry axis is +Y.
// The concave face opens toward +Y, with its rim in y=0 and vertex at
// y=-depth. A separate rear surface plus rim wall gives the antenna visible
// thickness without coincident triangles.
class ParabolicDishGenerator
{
public:
	static MeshData generate(float radius, float depth, float thickness = 0.01f,
		unsigned int radialSegments = 48, unsigned int radialRings = 8);
};

#endif
