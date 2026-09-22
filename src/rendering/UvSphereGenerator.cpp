#include "UvSphereGenerator.h"

#include <cmath>
#include <stdexcept>

#include <glm/geometric.hpp>

namespace
{
	constexpr float kPi = 3.14159265358979323846f;
	constexpr float kValidationEpsilon = 0.0001f;

	void validateSphere(const MeshData& data,
						unsigned int latitudeSegments,
						unsigned int longitudeSegments)
	{
		const std::size_t expectedVertices =
			static_cast<std::size_t>(latitudeSegments + 1) * (longitudeSegments + 1);
		const std::size_t expectedTriangles =
			2ull * longitudeSegments * (latitudeSegments - 1);

		if (data.vertices.size() != expectedVertices ||
			data.indices.size() != expectedTriangles * 3)
		{
			throw std::logic_error("UV sphere generated an unexpected vertex/index count");
		}

		for (const Vertex& vertex : data.vertices)
		{
			if (std::abs(glm::length(vertex.position) - 1.0f) > kValidationEpsilon ||
				std::abs(glm::length(vertex.normal) - 1.0f) > kValidationEpsilon ||
				vertex.texCoord.x < 0.0f || vertex.texCoord.x > 1.0f ||
				vertex.texCoord.y < 0.0f || vertex.texCoord.y > 1.0f)
			{
				throw std::logic_error("UV sphere generated an invalid vertex");
			}
		}

		for (std::size_t i = 0; i < data.indices.size(); i += 3)
		{
			const std::uint32_t ia = data.indices[i];
			const std::uint32_t ib = data.indices[i + 1];
			const std::uint32_t ic = data.indices[i + 2];
			if (ia >= data.vertices.size() || ib >= data.vertices.size() ||
				ic >= data.vertices.size())
			{
				throw std::logic_error("UV sphere generated an out-of-range index");
			}

			const glm::vec3 ab = data.vertices[ib].position - data.vertices[ia].position;
			const glm::vec3 ac = data.vertices[ic].position - data.vertices[ia].position;
			if (glm::length(glm::cross(ab, ac)) <= kValidationEpsilon)
				throw std::logic_error("UV sphere generated a degenerate triangle");
		}

		// At an outside surface point, increasing U follows up x normal. This
		// catches a horizontally mirrored procedural texture even though the
		// sphere itself is perfectly round.
		const std::size_t sample =
			static_cast<std::size_t>(latitudeSegments / 2) * (longitudeSegments + 1);
		const Vertex& current = data.vertices[sample];
		const Vertex& nextLongitude = data.vertices[sample + 1];
		const glm::vec3 longitudeStep =
			glm::normalize(nextLongitude.position - current.position);
		const float uDirection =
			nextLongitude.texCoord.x > current.texCoord.x ? 1.0f : -1.0f;
		const glm::vec3 geographicEast =
			glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), current.normal));
		if (glm::dot(longitudeStep * uDirection, geographicEast) <= 0.0f)
			throw std::logic_error("UV sphere texture longitude is horizontally mirrored");
	}
}

MeshData UvSphereGenerator::generate(unsigned int latitudeSegments,
									 unsigned int longitudeSegments)
{
	if (latitudeSegments < 2)
		throw std::invalid_argument("A UV sphere needs at least 2 latitude segments");
	if (longitudeSegments < 3)
		throw std::invalid_argument("A UV sphere needs at least 3 longitude segments");

	MeshData data;
	data.vertices.reserve(
		static_cast<std::size_t>(latitudeSegments + 1) * (longitudeSegments + 1));
	data.indices.reserve(
		static_cast<std::size_t>(6) * longitudeSegments * (latitudeSegments - 1));

	for (unsigned int latitude = 0; latitude <= latitudeSegments; ++latitude)
	{
		const float latitudeFraction =
			static_cast<float>(latitude) / static_cast<float>(latitudeSegments);
		const float polarAngle = kPi * latitudeFraction;
		const float ringRadius = std::sin(polarAngle);
		const float y = std::cos(polarAngle);

		for (unsigned int longitude = 0; longitude <= longitudeSegments; ++longitude)
		{
			const float longitudeFraction =
				static_cast<float>(longitude) / static_cast<float>(longitudeSegments);
			const float azimuth = 2.0f * kPi * longitudeFraction;

			Vertex vertex;
			vertex.position = glm::vec3(
				ringRadius * std::cos(azimuth),
				y,
				ringRadius * std::sin(azimuth));
			vertex.normal = glm::normalize(vertex.position);
			// Reverse U so eastward texture longitude follows the outside surface's
			// geographic-east tangent instead of appearing horizontally mirrored.
			vertex.texCoord = glm::vec2(
				1.0f - longitudeFraction, 1.0f - latitudeFraction);
			data.vertices.push_back(vertex);
		}
	}

	const unsigned int rowWidth = longitudeSegments + 1;
	for (unsigned int latitude = 0; latitude < latitudeSegments; ++latitude)
	{
		for (unsigned int longitude = 0; longitude < longitudeSegments; ++longitude)
		{
			const std::uint32_t topLeft = latitude * rowWidth + longitude;
			const std::uint32_t topRight = topLeft + 1;
			const std::uint32_t bottomLeft = topLeft + rowWidth;
			const std::uint32_t bottomRight = bottomLeft + 1;

			// Counter-clockwise from outside. Pole-degenerate halves are omitted.
			if (latitude > 0)
			{
				data.indices.push_back(topLeft);
				data.indices.push_back(topRight);
				data.indices.push_back(bottomLeft);
			}
			if (latitude + 1 < latitudeSegments)
			{
				data.indices.push_back(topRight);
				data.indices.push_back(bottomRight);
				data.indices.push_back(bottomLeft);
			}
		}
	}

	validateSphere(data, latitudeSegments, longitudeSegments);
	return data;
}
