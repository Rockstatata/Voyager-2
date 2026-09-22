#include "RingGenerator.h"

#include <cmath>
#include <stdexcept>

namespace
{
	constexpr float kPi = 3.14159265358979323846f;
}

MeshData RingGenerator::generate(float innerRadius, float outerRadius, unsigned int radialSegments)
{
	if (radialSegments < 3)
		throw std::invalid_argument("A ring needs at least 3 radial segments");
	if (innerRadius <= 0.0f || outerRadius <= innerRadius)
		throw std::invalid_argument("A ring needs 0 < innerRadius < outerRadius");

	MeshData data;
	const unsigned int rowWidth = radialSegments + 1;

	// Two faces, each with its own inner/outer ring of vertices (four rings
	// total) so each face gets its own correctly-signed normal — the same
	// per-face-normal tradeoff as every other generator in this codebase.
	for (unsigned int face = 0; face < 2; ++face)
	{
		const float normalY = face == 0 ? 1.0f : -1.0f;
		const std::uint32_t faceBase = static_cast<std::uint32_t>(data.vertices.size());

		for (unsigned int ring = 0; ring < 2; ++ring)
		{
			const float radius = ring == 0 ? innerRadius : outerRadius;
			for (unsigned int seg = 0; seg <= radialSegments; ++seg)
			{
				const float fraction = static_cast<float>(seg) / static_cast<float>(radialSegments);
				const float theta = 2.0f * kPi * fraction;

				Vertex vertex;
				vertex.position = glm::vec3(radius * std::cos(theta), 0.0f, radius * std::sin(theta));
				vertex.normal = glm::vec3(0.0f, normalY, 0.0f);
				vertex.texCoord = glm::vec2(fraction, static_cast<float>(ring));
				data.vertices.push_back(vertex);
			}
		}

		for (unsigned int seg = 0; seg < radialSegments; ++seg)
		{
			const std::uint32_t innerLeft = faceBase + seg;
			const std::uint32_t innerRight = faceBase + seg + 1;
			const std::uint32_t outerLeft = faceBase + rowWidth + seg;
			const std::uint32_t outerRight = faceBase + rowWidth + seg + 1;

			if (face == 0)
			{
				data.indices.push_back(innerLeft);
				data.indices.push_back(outerLeft);
				data.indices.push_back(innerRight);

				data.indices.push_back(innerRight);
				data.indices.push_back(outerLeft);
				data.indices.push_back(outerRight);
			}
			else
			{
				// Bottom face: reversed winding so it's still CCW from below.
				data.indices.push_back(innerLeft);
				data.indices.push_back(innerRight);
				data.indices.push_back(outerLeft);

				data.indices.push_back(innerRight);
				data.indices.push_back(outerRight);
				data.indices.push_back(outerLeft);
			}
		}
	}

	return data;
}
