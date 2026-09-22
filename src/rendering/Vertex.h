#ifndef VERTEX_H
#define VERTEX_H

#include <type_traits>

#include <glm/glm.hpp>

// One point in a renderable surface. Position defines shape, normal defines
// surface direction for future lighting, and texCoord selects an image pixel.
struct Vertex
{
	glm::vec3 position{ 0.0f };
	glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
	glm::vec2 texCoord{ 0.0f };
};

// Mesh::Mesh uses offsetof for the GPU attribute pointers. Keep this type
// standard-layout and tightly packed as eight floats (32 bytes).
static_assert(std::is_standard_layout_v<Vertex>);
static_assert(sizeof(Vertex) == sizeof(float) * 8);

#endif
