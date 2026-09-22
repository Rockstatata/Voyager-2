#ifndef RING_GENERATOR_H
#define RING_GENERATOR_H

#include "MeshData.h"

// Pure CPU geometry module. A flat annulus in the XZ plane (y=0), inner and
// outer radius given in the SAME "unit-sphere" convention as
// UvSphereGenerator: 1.0 means "one planet radius" so a ring can be added as
// a plain child of its planet's CelestialBody with no explicit scale of its
// own — the planet's existing transform().scale cascades onto it for free
// (bible section 12: worldMatrix = parent.worldMatrix() * child.localMatrix()
// already includes the parent's scale).
//
// Double-sided: since the free-fly camera can pass under the ring plane and
// this project has no per-mesh culling toggle, both faces are generated
// (mirrored winding/normals) rather than relying on GL_CULL_FACE state.
class RingGenerator
{
public:
	static MeshData generate(float innerRadius, float outerRadius, unsigned int radialSegments = 64);
};

#endif
