#ifndef TRIANGLE_BVH_H
#define TRIANGLE_BVH_H

#include <memory>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "MeshData.h"

class ShaderProgram;
struct Material;

// Ray tracing a triangle mesh on the GPU (docs/objects/ray-tracing.md).
//
// Voyager 2 is ~20,000 triangles. Testing every triangle for every ray would
// be far too slow, so the triangles are sorted into a Bounding Volume
// Hierarchy: a binary tree of axis-aligned boxes. A ray that misses a box
// skips every triangle inside it, so a typical ray tests a few dozen
// triangles instead of all of them.
//
// Build (CPU, once): gather every part's triangles in the spacecraft's local
// frame, then recursively split them at the median of the longest axis until
// a node holds at most four. Upload (GPU, once): nodes and triangles go into
// two buffer textures (samplerBuffer), read with texelFetch. Per frame only
// the spacecraft's transform is uploaded: rays are moved into its local
// frame instead of moving thousands of triangles.
class TriangleBvh
{
public:
	TriangleBvh() = default;
	~TriangleBvh();
	TriangleBvh(const TriangleBvh&) = delete;
	TriangleBvh& operator=(const TriangleBvh&) = delete;

	// Appends a part: its mesh, its local transform inside the spacecraft and
	// its material (which becomes an entry of the material palette).
	void addMesh(const MeshData& mesh, const glm::dmat4& localTransform, const std::shared_ptr<Material>& material);

	// Builds the tree and uploads it. Call once, with a GL context.
	void build();

	// Binds the buffers to texture units 3 and 4 (plus the albedo atlas to
	// unit 5) and uploads the traversal and material uniforms.
	void bind(ShaderProgram& program, const glm::dmat4& worldMatrix, const glm::dvec3& origin,
		double boundingRadius, bool enabled) const;

	std::size_t triangleCount() const { return m_triangles.size(); }
	std::size_t nodeCount() const { return m_nodes.size(); }
	int depth() const { return m_depth; }

	static constexpr int kMaxMaterials = 16;
	static constexpr int kTexelsPerTriangle = 7;

private:
	struct Triangle
	{
		glm::vec3 position[3];
		glm::vec3 normal[3];
		glm::vec2 uv[3];
		int material = 0;
		glm::vec3 centroid{ 0.0f };
	};
	struct Node
	{
		glm::vec3 boundsMin{ 0.0f };
		glm::vec3 boundsMax{ 0.0f };
		int first = 0;   // internal: left child; leaf: first triangle
		int second = 0;  // internal: right child; leaf: -(triangle count)
	};

	int buildNode(int begin, int end, int depth);
	void release();

	std::vector<Triangle> m_triangles;
	std::vector<Node> m_nodes;
	std::vector<std::shared_ptr<Material>> m_palette;
	int m_depth = 0;

	GLuint m_nodeBuffer = 0;
	GLuint m_nodeTexture = 0;
	GLuint m_triangleBuffer = 0;
	GLuint m_triangleTexture = 0;
};

#endif
