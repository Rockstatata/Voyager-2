#include "TriangleBvh.h"

#include <algorithm>
#include <iostream>
#include <string>

#include "Material.h"
#include "ShaderProgram.h"

TriangleBvh::~TriangleBvh()
{
	release();
}

void TriangleBvh::release()
{
	if (m_nodeTexture != 0)
		glDeleteTextures(1, &m_nodeTexture);
	if (m_triangleTexture != 0)
		glDeleteTextures(1, &m_triangleTexture);
	if (m_nodeBuffer != 0)
		glDeleteBuffers(1, &m_nodeBuffer);
	if (m_triangleBuffer != 0)
		glDeleteBuffers(1, &m_triangleBuffer);
	m_nodeTexture = m_triangleTexture = m_nodeBuffer = m_triangleBuffer = 0;
}

void TriangleBvh::addMesh(const MeshData& mesh, const glm::dmat4& localTransform,
	const std::shared_ptr<Material>& material)
{
	// Palette slot for this material (shared materials share a slot).
	auto found = std::find(m_palette.begin(), m_palette.end(), material);
	int materialIndex = static_cast<int>(found - m_palette.begin());
	if (found == m_palette.end())
	{
		if (static_cast<int>(m_palette.size()) >= kMaxMaterials)
			materialIndex = kMaxMaterials - 1; // shares the last slot rather than failing
		else
			m_palette.push_back(material);
	}

	const glm::dmat3 normalMatrix = glm::transpose(glm::inverse(glm::dmat3(localTransform)));
	for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
	{
		Triangle triangle;
		for (int corner = 0; corner < 3; ++corner)
		{
			const Vertex& vertex = mesh.vertices[mesh.indices[i + corner]];
			triangle.position[corner] = glm::vec3(localTransform * glm::dvec4(vertex.position, 1.0));
			triangle.normal[corner] = glm::normalize(glm::vec3(normalMatrix * glm::dvec3(vertex.normal)));
			triangle.uv[corner] = vertex.texCoord;
		}
		triangle.material = materialIndex;
		triangle.centroid = (triangle.position[0] + triangle.position[1] + triangle.position[2]) / 3.0f;
		m_triangles.push_back(triangle);
	}
}

int TriangleBvh::buildNode(int begin, int end, int depth)
{
	m_depth = std::max(m_depth, depth);
	const int index = static_cast<int>(m_nodes.size());
	m_nodes.push_back({});

	// Box around every vertex of the triangles in [begin, end).
	glm::vec3 boundsMin(1e30f);
	glm::vec3 boundsMax(-1e30f);
	glm::vec3 centroidMin(1e30f);
	glm::vec3 centroidMax(-1e30f);
	for (int i = begin; i < end; ++i)
	{
		for (const glm::vec3& p : m_triangles[i].position)
		{
			boundsMin = glm::min(boundsMin, p);
			boundsMax = glm::max(boundsMax, p);
		}
		centroidMin = glm::min(centroidMin, m_triangles[i].centroid);
		centroidMax = glm::max(centroidMax, m_triangles[i].centroid);
	}
	m_nodes[index].boundsMin = boundsMin;
	m_nodes[index].boundsMax = boundsMax;

	const int count = end - begin;
	const glm::vec3 extent = centroidMax - centroidMin;
	if (count <= 4 || std::max({ extent.x, extent.y, extent.z }) <= 0.0f)
	{
		m_nodes[index].first = begin;
		m_nodes[index].second = -count;
		return index;
	}

	// Split at the median centroid along the longest axis: each child gets
	// half the triangles, so the tree stays balanced (depth ~ log2(n / 4)).
	const int axis = extent.x > extent.y ? (extent.x > extent.z ? 0 : 2) : (extent.y > extent.z ? 1 : 2);
	const int middle = begin + count / 2;
	std::nth_element(m_triangles.begin() + begin, m_triangles.begin() + middle, m_triangles.begin() + end,
		[axis](const Triangle& a, const Triangle& b) { return a.centroid[axis] < b.centroid[axis]; });

	const int left = buildNode(begin, middle, depth + 1);
	const int right = buildNode(middle, end, depth + 1);
	m_nodes[index].first = left;
	m_nodes[index].second = right;
	return index;
}

void TriangleBvh::build()
{
	m_nodes.clear();
	m_depth = 0;
	if (m_triangles.empty())
		return;
	buildNode(0, static_cast<int>(m_triangles.size()), 0);

	// Nodes: 2 RGBA32F texels each: (min.xyz, first), (max.xyz, second).
	std::vector<glm::vec4> nodeTexels;
	nodeTexels.reserve(m_nodes.size() * 2);
	for (const Node& node : m_nodes)
	{
		nodeTexels.emplace_back(node.boundsMin, static_cast<float>(node.first));
		nodeTexels.emplace_back(node.boundsMax, static_cast<float>(node.second));
	}

	// Triangles: 7 texels each (see raytrace_mesh.glsl for the layout).
	std::vector<glm::vec4> triangleTexels;
	triangleTexels.reserve(m_triangles.size() * kTexelsPerTriangle);
	for (const Triangle& t : m_triangles)
	{
		triangleTexels.emplace_back(t.position[0], static_cast<float>(t.material));
		triangleTexels.emplace_back(t.position[1], 0.0f);
		triangleTexels.emplace_back(t.position[2], 0.0f);
		triangleTexels.emplace_back(t.normal[0], t.uv[0].x);
		triangleTexels.emplace_back(t.normal[1], t.uv[0].y);
		triangleTexels.emplace_back(t.normal[2], t.uv[1].x);
		triangleTexels.emplace_back(t.uv[1].y, t.uv[2].x, t.uv[2].y, 0.0f);
	}

	release();
	auto upload = [](const std::vector<glm::vec4>& texels, GLuint& buffer, GLuint& texture)
	{
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_TEXTURE_BUFFER, buffer);
		glBufferData(GL_TEXTURE_BUFFER, static_cast<GLsizeiptr>(texels.size() * sizeof(glm::vec4)),
			texels.data(), GL_STATIC_DRAW);
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_BUFFER, texture);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, buffer);
		glBindTexture(GL_TEXTURE_BUFFER, 0);
		glBindBuffer(GL_TEXTURE_BUFFER, 0);
	};
	upload(nodeTexels, m_nodeBuffer, m_nodeTexture);
	upload(triangleTexels, m_triangleBuffer, m_triangleTexture);

	std::cout << "[SHADER] BVH built: " << m_triangles.size() << " triangles, " << m_nodes.size()
			  << " nodes, depth " << m_depth << ", " << m_palette.size() << " materials" << std::endl;
}

void TriangleBvh::bind(ShaderProgram& program, const glm::dmat4& worldMatrix, const glm::dvec3& origin,
	double boundingRadius, bool enabled) const
{
	const bool usable = enabled && m_nodeTexture != 0;
	glUniform1i(program.uniform("meshEnabled"), usable ? 1 : 0);
	if (!usable)
		return;

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_BUFFER, m_nodeTexture);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_BUFFER, m_triangleTexture);
	glUniform1i(program.uniform("bvhNodes"), 3);
	glUniform1i(program.uniform("bvhTriangles"), 4);

	// Rigid transform only (Voyager has scale 1): rotation columns are unit
	// axes, so world-to-local is the transpose of the rotation.
	const glm::vec3 position(glm::dvec3(worldMatrix[3]) - origin);
	const glm::mat3 worldToLocal = glm::transpose(glm::mat3(glm::dmat3(worldMatrix)));
	glUniform3fv(program.uniform("meshPosition"), 1, &position[0]);
	glUniformMatrix3fv(program.uniform("meshWorldToLocal"), 1, GL_FALSE, &worldToLocal[0][0]);
	glUniform1f(program.uniform("meshBoundingRadius"), static_cast<float>(boundingRadius));

	// Material palette for traced hits (the ray-traced view shades them).
	const Texture2D* atlas = nullptr;
	for (int i = 0; i < static_cast<int>(m_palette.size()); ++i)
	{
		const Material& material = *m_palette[i];
		const std::string index = "[" + std::to_string(i) + "]";
		const bool textured = material.albedoTexture != nullptr && material.albedoTexture->valid();
		if (textured && atlas == nullptr)
			atlas = material.albedoTexture.get();
		const glm::vec2 specular(material.specularStrength, material.specularPower);
		glUniform3fv(program.uniform("meshMaterialColor" + index), 1, &material.baseColor[0]);
		glUniform2fv(program.uniform("meshMaterialSpecular" + index), 1, &specular[0]);
		glUniform4fv(program.uniform("meshMaterialUv" + index), 1, &material.uvTransform[0]);
		glUniform1i(program.uniform("meshMaterialTextured" + index), textured ? 1 : 0);
	}
	if (atlas != nullptr)
	{
		atlas->bind(5);
		glUniform1i(program.uniform("meshAtlas"), 5);
	}
	glActiveTexture(GL_TEXTURE0);
}
