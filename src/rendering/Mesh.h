#ifndef MESH_H
#define MESH_H

#include <memory>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "../../VAO.h"
#include "../../VBO.h"
#include "../../EBO.h"
#include "MeshData.h"

// What glDrawElements assembles the index buffer into. MeshData itself stays
// GL-free (bible: pure geometry generators are independent of OpenGL); this
// enum lives here, at the boundary where geometry actually becomes a draw
// call, not in MeshData.
enum class PrimitiveMode { Triangles, Lines, LineStrip, LineLoop, Points };

// GPU-side indexed geometry built on the existing VAO/VBO/EBO wrappers.
// Layout: location 0 = position, 1 = normal, 2 = texture coordinate.
//
// The wrappers are held by value and are themselves RAII, so Mesh needs no
// destructor and no raw new/delete. Move-only, because its members are.
class Mesh
{
public:
	explicit Mesh(const MeshData& data, PrimitiveMode mode = PrimitiveMode::Triangles);

	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;
	Mesh(Mesh&&) noexcept = default;
	Mesh& operator=(Mesh&&) noexcept = default;

	void draw() const;

	// Draws `instanceCount` copies in one GPU call, each transformed by the
	// matching entry of `instanceModelMatrices` (uploaded once by
	// setInstanceTransforms, not per frame) — the mechanism that lets an
	// asteroid/Kuiper/Oort belt exist as thousands of bodies without
	// thousands of draw calls (bible failure mode F9).
	void setInstanceTransforms(const std::vector<glm::mat4>& instanceModelMatrices);
	void drawInstanced() const;

	GLsizei indexCount() const { return m_indexCount; }
	GLsizei vertexCount() const { return m_vertexCount; }

private:
	GLenum glPrimitive() const;

	VAO m_vao;
	VBO m_vbo;
	EBO m_ebo;
	GLsizei m_vertexCount = 0;
	GLsizei m_indexCount = 0;
	PrimitiveMode m_primitiveMode = PrimitiveMode::Triangles;

	// Only allocated if setInstanceTransforms is called.
	std::unique_ptr<VBO> m_instanceVbo;
	GLsizei m_instanceCount = 0;
};

#endif
