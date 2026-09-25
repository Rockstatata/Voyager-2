#ifndef VOYAGER_MODEL_BUILDER_H
#define VOYAGER_MODEL_BUILDER_H

#include <cstddef>
#include <memory>

class TriangleBvh;
class Voyager2;

struct VoyagerModelBuildResult
{
	std::unique_ptr<Voyager2> spacecraft;
	std::size_t visiblePartCount = 0;
	std::size_t renderedTriangleCount = 0;
	// Every visible triangle in the spacecraft's frame, for ray tracing.
	std::shared_ptr<TriangleBvh> traceMesh;
};

// Owns the procedural construction recipe for the spacecraft's visible
// hardware. Nothing is imported from the NASA reference model: every vertex
// comes from the project's own generators and every assembly is explicit.
class VoyagerModelBuilder
{
public:
	static VoyagerModelBuildResult build();
};

#endif
