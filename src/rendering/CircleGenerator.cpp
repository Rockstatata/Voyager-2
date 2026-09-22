#include "CircleGenerator.h"

#include <cmath>
#include <stdexcept>

namespace
{
	constexpr float kPi = 3.14159265358979323846f;
}

MeshData CircleGenerator::generate(unsigned int segments)
{
	if (segments < 3)
		throw std::invalid_argument("A circle needs at least 3 segments");

	MeshData data;
	data.vertices.reserve(segments);
	data.indices.reserve(segments);

	for (unsigned int seg = 0; seg < segments; ++seg)
	{
		const float fraction = static_cast<float>(seg) / static_cast<float>(segments);
		const float theta = 2.0f * kPi * fraction;

		Vertex vertex;
		vertex.position = glm::vec3(std::cos(theta), 0.0f, std::sin(theta));
		vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
		vertex.texCoord = glm::vec2(fraction, 0.0f);
		data.vertices.push_back(vertex);
		data.indices.push_back(seg); // GL_LINE_LOOP closes the last->first edge itself
	}

	return data;
}
