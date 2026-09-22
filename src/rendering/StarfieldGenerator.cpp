#include "StarfieldGenerator.h"

#include <cmath>
#include <random>

MeshData StarfieldGenerator::generate(unsigned int count, float radius, unsigned int seed)
{
	MeshData data;
	data.vertices.reserve(count);
	data.indices.reserve(count);

	// Fixed seed: deterministic, same reasoning as this project's other
	// generated content (procedural material, etc) — reproducible builds.
	std::mt19937 rng(seed);
	std::uniform_real_distribution<float> unit(0.0f, 1.0f);

	for (unsigned int i = 0; i < count; ++i)
	{
		// Uniform point on a sphere: theta uniform in [0, 2*pi), and cos(phi)
		// uniform in [-1, 1] rather than phi itself — sampling phi directly
		// would bunch points near the poles the same way UV-sphere latitude
		// rings shrink near the poles, just without geometry to compensate.
		const float theta = 2.0f * 3.14159265358979323846f * unit(rng);
		const float cosPhi = 2.0f * unit(rng) - 1.0f;
		const float sinPhi = std::sqrt(1.0f - cosPhi * cosPhi);

		Vertex vertex;
		vertex.position = glm::vec3(radius * sinPhi * std::cos(theta),
									 radius * cosPhi,
									 radius * sinPhi * std::sin(theta));
		vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f); // unused for points
		vertex.texCoord = glm::vec2(0.0f, 0.0f);     // unused, no texture
		data.vertices.push_back(vertex);
		data.indices.push_back(static_cast<std::uint32_t>(i));
	}

	return data;
}
