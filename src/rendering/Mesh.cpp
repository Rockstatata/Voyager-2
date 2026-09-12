#include "Mesh.h"

Mesh::Mesh(const GLfloat* vertices, GLsizeiptr verticesSize,
		   const GLuint* indices, GLsizeiptr indicesSize, GLsizei indexCount)
	: m_vao(), m_vbo(vertices, verticesSize), m_ebo(indices, indicesSize), m_indexCount(indexCount)
{
	// The buffers were created before the VAO was bound, so the element-array
	// binding must be recorded into the VAO here: GL_ELEMENT_ARRAY_BUFFER is
	// part of vertex-array state, GL_ARRAY_BUFFER is not.
	m_vao.Bind();
	m_ebo.Bind();

	const GLsizeiptr stride = 6 * sizeof(GLfloat);
	m_vao.LinkAttrib(m_vbo, 0, 3, GL_FLOAT, stride, (void*)0);
	m_vao.LinkAttrib(m_vbo, 1, 3, GL_FLOAT, stride, (void*)(3 * sizeof(GLfloat)));

	// Unbind the VAO first; unbinding the EBO while the VAO is still bound
	// would erase the element-array binding we just recorded.
	m_vao.Unbind();
	m_vbo.Unbind();
	m_ebo.Unbind();
}

void Mesh::draw() const
{
	if (m_vao.ID == 0)
		return;

	m_vao.Bind();
	glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
	m_vao.Unbind();
}
