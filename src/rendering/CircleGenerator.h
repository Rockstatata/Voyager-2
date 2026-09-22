#ifndef CIRCLE_GENERATOR_H
#define CIRCLE_GENERATOR_H

#include "MeshData.h"

// Pure CPU geometry module. A unit circle (radius 1) in the local XZ plane,
// meant to be drawn with PrimitiveMode::LineLoop (Mesh.h) — a faint orbit
// path preview. "Unit" so one shared mesh serves every orbit: each ring's
// SceneObject sets its own transform().scale to the actual orbit radius,
// the same shared-geometry-scaled-per-instance pattern as UvSphereGenerator.
class CircleGenerator
{
public:
	static MeshData generate(unsigned int segments = 96);
};

#endif
