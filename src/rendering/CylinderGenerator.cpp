#include "CylinderGenerator.h"

#include <cmath>
#include <stdexcept>

namespace
{
	constexpr float kPi = 3.14159265358979323846f;
}

MeshData CylinderGenerator::generate(float radiusBottom, float radiusTop, float height,
									 unsigned int radialSegments, bool capBottom, bool capTop)
{
	if (radialSegments < 3)
		throw std::invalid_argument("A cylinder needs at least 3 radial segments");

	MeshData data;
	const float halfHeight = height * 0.5f;
	const float deltaRadius = radiusTop - radiusBottom;

	// --- Lateral surface: two rings (bottom, top), seam column duplicated. ---
	const unsigned int rowWidth = radialSegments + 1;
	for (unsigned int ring = 0; ring < 2; ++ring)
	{
		const float y = ring == 0 ? -halfHeight : halfHeight;
		const float radius = ring == 0 ? radiusBottom : radiusTop;

		for (unsigned int seg = 0; seg <= radialSegments; ++seg)
		{
			const float fraction = static_cast<float>(seg) / static_cast<float>(radialSegments);
			const float theta = 2.0f * kPi * fraction;
			const float cosT = std::cos(theta);
			const float sinT = std::sin(theta);

			Vertex vertex;
			vertex.position = glm::vec3(radius * cosT, y, radius * sinT);
			// Outward normal from the generatrix (bottom-to-top edge) crossed
			// with the circumferential tangent; see CylinderGenerator.h.
			vertex.normal = glm::normalize(glm::vec3(height * cosT, -deltaRadius, height * sinT));
			vertex.texCoord = glm::vec2(1.0f - fraction, static_cast<float>(ring));
			data.vertices.push_back(vertex);
		}
	}

	if (radiusBottom > 0.0f && radiusTop > 0.0f) // a true cone's apex ring has zero-area quads; skip
	{
		for (unsigned int seg = 0; seg < radialSegments; ++seg)
		{
			const std::uint32_t bottomLeft = seg;
			const std::uint32_t bottomRight = seg + 1;
			const std::uint32_t topLeft = rowWidth + seg;
			const std::uint32_t topRight = rowWidth + seg + 1;

			data.indices.push_back(bottomLeft);
			data.indices.push_back(bottomRight);
			data.indices.push_back(topLeft);

			data.indices.push_back(bottomRight);
			data.indices.push_back(topRight);
			data.indices.push_back(topLeft);
		}
	}
	else
	{
		// One radius is zero (a true cone/apex): one triangle per segment,
		// from the surviving ring to the coincident-position apex vertices.
		for (unsigned int seg = 0; seg < radialSegments; ++seg)
		{
			const std::uint32_t a = seg;
			const std::uint32_t b = seg + 1;
			const std::uint32_t c = rowWidth + seg;
			if (radiusBottom > 0.0f)
			{
				data.indices.push_back(a);
				data.indices.push_back(b);
				data.indices.push_back(c);
			}
			else
			{
				data.indices.push_back(a);
				data.indices.push_back(c);
				data.indices.push_back(rowWidth + seg + 1);
			}
		}
	}

	// --- Caps: independent fan of vertices (flat normal), same seam-duplication
	// tradeoff as the lateral surface but for a different reason — a cap vertex
	// needs a different normal than the lateral vertex at the same position. ---
	auto addCap = [&](float y, float radius, bool isTop)
	{
		if (radius <= 0.0f)
			return;

		const std::uint32_t centerIndex = static_cast<std::uint32_t>(data.vertices.size());
		Vertex center;
		center.position = glm::vec3(0.0f, y, 0.0f);
		center.normal = glm::vec3(0.0f, isTop ? 1.0f : -1.0f, 0.0f);
		center.texCoord = glm::vec2(0.5f, 0.5f);
		data.vertices.push_back(center);

		const std::uint32_t ringStart = static_cast<std::uint32_t>(data.vertices.size());
		for (unsigned int seg = 0; seg <= radialSegments; ++seg)
		{
			const float fraction = static_cast<float>(seg) / static_cast<float>(radialSegments);
			const float theta = 2.0f * kPi * fraction;
			Vertex vertex;
			vertex.position = glm::vec3(radius * std::cos(theta), y, radius * std::sin(theta));
			vertex.normal = center.normal;
			vertex.texCoord = glm::vec2(0.5f + 0.5f * std::cos(theta), 0.5f + 0.5f * std::sin(theta));
			data.vertices.push_back(vertex);
		}

		for (unsigned int seg = 0; seg < radialSegments; ++seg)
		{
			const std::uint32_t a = ringStart + seg;
			const std::uint32_t b = ringStart + seg + 1;
			if (isTop)
			{
				data.indices.push_back(centerIndex);
				data.indices.push_back(a);
				data.indices.push_back(b);
			}
			else
			{
				data.indices.push_back(centerIndex);
				data.indices.push_back(b);
				data.indices.push_back(a);
			}
		}
	};

	if (capBottom)
		addCap(-halfHeight, radiusBottom, false);
	if (capTop)
		addCap(halfHeight, radiusTop, true);

	return data;
}
