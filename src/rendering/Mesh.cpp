#include "Mesh.h"

Mesh::Mesh(const GLfloat* vertices, GLsizeiptr verticesSize,
		   const GLuint* indices, GLsizeiptr indicesSize, GLsizei indexCount)
	: m_indexCount(indexCount)
{
	m_vao = new VAO();
	m_vao->Bind();

	m_vbo = new VBO(const_cast<GLfloat*>(vertices), verticesSize);
	m_ebo = new EBO(const_cast<GLuint*>(indices), indicesSize);

	m_vao->LinkAttrib(*m_vbo, 0, 3, GL_FLOAT, 6 * sizeof(GL_FLOAT), (void*)0);
	m_vao->LinkAttrib(*m_vbo, 1, 3, GL_FLOAT, 6 * sizeof(GL_FLOAT), (void*)(3 * sizeof(GL_FLOAT)));

	m_vao->Unbind();
	m_vbo->Unbind();
	m_ebo->Unbind();
}

Mesh::~Mesh()
{
	release();
}

Mesh::Mesh(Mesh&& other) noexcept
	: m_vao(other.m_vao), m_vbo(other.m_vbo), m_ebo(other.m_ebo), m_indexCount(other.m_indexCount)
{
	other.m_vao = nullptr;
	other.m_vbo = nullptr;
	other.m_ebo = nullptr;
	other.m_indexCount = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
	if (this != &other)
	{
		release();
		m_vao = other.m_vao;
		m_vbo = other.m_vbo;
		m_ebo = other.m_ebo;
		m_indexCount = other.m_indexCount;
		other.m_vao = nullptr;
		other.m_vbo = nullptr;
		other.m_ebo = nullptr;
		other.m_indexCount = 0;
	}
	return *this;
}

void Mesh::draw() const
{
	if (m_vao == nullptr)
		return;
	m_vao->Bind();
	glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
	m_vao->Unbind();
}

void Mesh::release()
{
	if (m_vao != nullptr) { m_vao->Delete(); delete m_vao; m_vao = nullptr; }
	if (m_vbo != nullptr) { m_vbo->Delete(); delete m_vbo; m_vbo = nullptr; }
	if (m_ebo != nullptr) { m_ebo->Delete(); delete m_ebo; m_ebo = nullptr; }
	m_indexCount = 0;
}
