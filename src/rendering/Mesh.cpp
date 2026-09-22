#include "Mesh.h"

#include <cstddef>

Mesh::Mesh(const MeshData& data, PrimitiveMode mode)
	: m_vao(),
	  m_vbo(data.vertices.data(), static_cast<GLsizeiptr>(data.vertices.size() * sizeof(Vertex))),
	  m_ebo(data.indices.data(), static_cast<GLsizeiptr>(data.indices.size() * sizeof(std::uint32_t))),
	  m_vertexCount(static_cast<GLsizei>(data.vertices.size())),
	  m_indexCount(static_cast<GLsizei>(data.indices.size())),
	  m_primitiveMode(mode)
{
	// The buffers were created before the VAO was bound, so the element-array
	// binding must be recorded into the VAO here: GL_ELEMENT_ARRAY_BUFFER is
	// part of vertex-array state, GL_ARRAY_BUFFER is not.
	m_vao.Bind();
	m_ebo.Bind();

	const GLsizeiptr stride = sizeof(Vertex);
	m_vao.LinkAttrib(m_vbo, 0, 3, GL_FLOAT, stride,
					 reinterpret_cast<const void*>(offsetof(Vertex, position)));
	m_vao.LinkAttrib(m_vbo, 1, 3, GL_FLOAT, stride,
					 reinterpret_cast<const void*>(offsetof(Vertex, normal)));
	m_vao.LinkAttrib(m_vbo, 2, 2, GL_FLOAT, stride,
					 reinterpret_cast<const void*>(offsetof(Vertex, texCoord)));

	// Unbind the VAO first; unbinding the EBO while the VAO is still bound
	// would erase the element-array binding we just recorded.
	m_vao.Unbind();
	m_vbo.Unbind();
	m_ebo.Unbind();
}

GLenum Mesh::glPrimitive() const
{
	switch (m_primitiveMode)
	{
		case PrimitiveMode::Lines: return GL_LINES;
		case PrimitiveMode::LineStrip: return GL_LINE_STRIP;
		case PrimitiveMode::LineLoop: return GL_LINE_LOOP;
		case PrimitiveMode::Points: return GL_POINTS;
		default: return GL_TRIANGLES;
	}
}

void Mesh::draw() const
{
	if (m_vao.ID == 0)
		return;

	m_vao.Bind();
	glDrawElements(glPrimitive(), m_indexCount, GL_UNSIGNED_INT, nullptr);
	m_vao.Unbind();
}

void Mesh::setInstanceTransforms(const std::vector<glm::mat4>& instanceModelMatrices)
{
	m_instanceCount = static_cast<GLsizei>(instanceModelMatrices.size());
	m_instanceVbo = std::make_unique<VBO>(instanceModelMatrices.data(),
		static_cast<GLsizeiptr>(instanceModelMatrices.size() * sizeof(glm::mat4)));

	// A mat4 attribute doesn't exist as one GL type: it's uploaded as four
	// consecutive vec4 attributes (locations 3-6, following aPos/aNormal/
	// aTexCoord at 0-2 — see default.vert). glVertexAttribDivisor(loc, 1)
	// is what makes each of those four attributes advance once per INSTANCE
	// instead of once per vertex, the actual mechanism behind "draw N copies
	// in one call, each with its own transform" (bible failure mode F9).
	m_vao.Bind();
	m_instanceVbo->Bind();
	for (GLuint column = 0; column < 4; ++column)
	{
		const GLuint location = 3 + column;
		glEnableVertexAttribArray(location);
		glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
			reinterpret_cast<const void*>(sizeof(glm::vec4) * column));
		glVertexAttribDivisor(location, 1);
	}
	m_vao.Unbind();
	m_instanceVbo->Unbind();
}

void Mesh::drawInstanced() const
{
	if (m_vao.ID == 0 || m_instanceVbo == nullptr || m_instanceCount == 0)
		return;

	m_vao.Bind();
	glDrawElementsInstanced(glPrimitive(), m_indexCount, GL_UNSIGNED_INT, nullptr, m_instanceCount);
	m_vao.Unbind();
}
