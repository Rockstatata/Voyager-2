#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>

#include "../../VAO.h"
#include "../../VBO.h"
#include "../../EBO.h"

// Minimal renderable geometry built on the existing VAO/VBO/EBO wrappers.
// Layout matches the starter shader: location 0 = position (vec3),
// location 1 = color (vec3).
//
// The wrappers are held by value and are themselves RAII, so Mesh needs no
// destructor and no raw new/delete. Move-only, because its members are.
class Mesh
{
public:
	Mesh(const GLfloat* vertices, GLsizeiptr verticesSize,
		 const GLuint* indices, GLsizeiptr indicesSize, GLsizei indexCount);

	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;
	Mesh(Mesh&&) noexcept = default;
	Mesh& operator=(Mesh&&) noexcept = default;

	void draw() const;

	GLsizei indexCount() const { return m_indexCount; }

private:
	VAO m_vao;
	VBO m_vbo;
	EBO m_ebo;
	GLsizei m_indexCount = 0;
};

#endif
