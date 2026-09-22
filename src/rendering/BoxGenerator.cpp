#include "BoxGenerator.h"

#include <array>
#include <stdexcept>

MeshData BoxGenerator::generate(float width, float height, float depth)
{
	if (width <= 0.0f || height <= 0.0f || depth <= 0.0f)
		throw std::invalid_argument("A box needs positive width, height, and depth");

	const float x = width * 0.5f;
	const float y = height * 0.5f;
	const float z = depth * 0.5f;

	struct Face
	{
		glm::vec3 normal;
		std::array<glm::vec3, 4> corners;
	};

	// Every corner list is counter-clockwise when viewed from outside.
	const std::array<Face, 6> faces = {{
		{ { 1.0f, 0.0f, 0.0f }, {{{ x, -y, -z }, { x, y, -z }, { x, y, z }, { x, -y, z }}} },
		{ {-1.0f, 0.0f, 0.0f }, {{{-x, -y, z }, {-x, y, z }, {-x, y, -z }, {-x, -y, -z }}} },
		{ { 0.0f, 1.0f, 0.0f }, {{{-x, y, -z }, {-x, y, z }, { x, y, z }, { x, y, -z }}} },
		{ { 0.0f,-1.0f, 0.0f }, {{{-x, -y, z }, {-x, -y, -z }, { x, -y, -z }, { x, -y, z }}} },
		{ { 0.0f, 0.0f, 1.0f }, {{{ x, -y, z }, { x, y, z }, {-x, y, z }, {-x, -y, z }}} },
		{ { 0.0f, 0.0f,-1.0f }, {{{-x, -y, -z }, {-x, y, -z }, { x, y, -z }, { x, -y, -z }}} },
	}};

	const std::array<glm::vec2, 4> uvs = {{
		{ 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f }
	}};

	MeshData data;
	data.vertices.reserve(24);
	data.indices.reserve(36);

	for (const Face& face : faces)
	{
		const std::uint32_t base = static_cast<std::uint32_t>(data.vertices.size());
		for (std::size_t i = 0; i < face.corners.size(); ++i)
			data.vertices.push_back({ face.corners[i], face.normal, uvs[i] });

		data.indices.insert(data.indices.end(), {
			base, base + 1, base + 2,
			base, base + 2, base + 3
		});
	}

	return data;
}
