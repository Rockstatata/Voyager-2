#include "ParabolicDishGenerator.h"

#include <cmath>
#include <stdexcept>

namespace
{
	constexpr float kTwoPi = 6.28318530717958647692f;

	void appendSurface(MeshData& data, float radius, float depth, float yOffset,
		unsigned int radialSegments, unsigned int radialRings, bool front)
	{
		const std::uint32_t center = static_cast<std::uint32_t>(data.vertices.size());
		Vertex centerVertex;
		centerVertex.position = glm::vec3(0.0f, -depth + yOffset, 0.0f);
		centerVertex.normal = glm::vec3(0.0f, front ? 1.0f : -1.0f, 0.0f);
		centerVertex.texCoord = glm::vec2(0.5f);
		data.vertices.push_back(centerVertex);

		const unsigned int rowWidth = radialSegments + 1;
		for (unsigned int ring = 1; ring <= radialRings; ++ring)
		{
			const float radialFraction = static_cast<float>(ring) / static_cast<float>(radialRings);
			const float r = radius * radialFraction;
			const float y = -depth + depth * radialFraction * radialFraction + yOffset;
			const float slope = 2.0f * depth * r / (radius * radius);

			for (unsigned int segment = 0; segment <= radialSegments; ++segment)
			{
				const float fraction = static_cast<float>(segment) / static_cast<float>(radialSegments);
				const float theta = kTwoPi * fraction;
				const float cosTheta = std::cos(theta);
				const float sinTheta = std::sin(theta);
				glm::vec3 normal = glm::normalize(glm::vec3(-slope * cosTheta, 1.0f, -slope * sinTheta));
				if (!front)
					normal = -normal;

				Vertex vertex;
				vertex.position = glm::vec3(r * cosTheta, y, r * sinTheta);
				vertex.normal = normal;
				vertex.texCoord = glm::vec2(0.5f + 0.5f * radialFraction * cosTheta,
					0.5f + 0.5f * radialFraction * sinTheta);
				data.vertices.push_back(vertex);
			}
		}

		const std::uint32_t firstRing = center + 1;
		for (unsigned int segment = 0; segment < radialSegments; ++segment)
		{
			const std::uint32_t a = firstRing + segment;
			const std::uint32_t b = firstRing + segment + 1;
			if (front)
				data.indices.insert(data.indices.end(), { center, b, a });
			else
				data.indices.insert(data.indices.end(), { center, a, b });
		}

		for (unsigned int ring = 1; ring < radialRings; ++ring)
		{
			const std::uint32_t inner = firstRing + (ring - 1) * rowWidth;
			const std::uint32_t outer = inner + rowWidth;
			for (unsigned int segment = 0; segment < radialSegments; ++segment)
			{
				const std::uint32_t innerLeft = inner + segment;
				const std::uint32_t innerRight = innerLeft + 1;
				const std::uint32_t outerLeft = outer + segment;
				const std::uint32_t outerRight = outerLeft + 1;
				if (front)
				{
					data.indices.insert(data.indices.end(), {
						innerLeft, innerRight, outerLeft,
						innerRight, outerRight, outerLeft
					});
				}
				else
				{
					data.indices.insert(data.indices.end(), {
						innerLeft, outerLeft, innerRight,
						innerRight, outerLeft, outerRight
					});
				}
			}
		}
	}
}

MeshData ParabolicDishGenerator::generate(float radius, float depth, float thickness,
	unsigned int radialSegments, unsigned int radialRings)
{
	if (radius <= 0.0f || depth <= 0.0f || thickness <= 0.0f)
		throw std::invalid_argument("A parabolic dish needs positive dimensions");
	if (radialSegments < 8 || radialRings < 2)
		throw std::invalid_argument("A parabolic dish needs at least 8 segments and 2 rings");

	MeshData data;
	appendSurface(data, radius, depth, 0.0f, radialSegments, radialRings, true);
	appendSurface(data, radius, depth, -thickness, radialSegments, radialRings, false);

	// Join only the two outermost rings. Their faces are separated by
	// `thickness`, so the shell is watertight without overlapping the bowl.
	const unsigned int rowWidth = radialSegments + 1;
	const std::uint32_t frontOuter = 1 + (radialRings - 1) * rowWidth;
	const std::uint32_t backCenter = 1 + radialRings * rowWidth;
	const std::uint32_t backOuter = backCenter + 1 + (radialRings - 1) * rowWidth;
	for (unsigned int segment = 0; segment < radialSegments; ++segment)
	{
		const std::uint32_t frontLeft = frontOuter + segment;
		const std::uint32_t frontRight = frontLeft + 1;
		const std::uint32_t backLeft = backOuter + segment;
		const std::uint32_t backRight = backLeft + 1;
		data.indices.insert(data.indices.end(), {
			frontLeft, backLeft, frontRight,
			frontRight, backLeft, backRight
		});
	}

	return data;
}
