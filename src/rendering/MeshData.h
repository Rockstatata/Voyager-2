#ifndef MESH_DATA_H
#define MESH_DATA_H

#include <cstdint>
#include <vector>

#include "Vertex.h"

// CPU-side geometry. Generators produce it; Mesh uploads it to the GPU.
struct MeshData
{
	std::vector<Vertex> vertices;
	std::vector<std::uint32_t> indices;
};

#endif
