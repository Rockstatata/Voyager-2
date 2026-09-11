#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <vector>

#include "../../VAO.h"
#include "../../VBO.h"
#include "../../EBO.h"

// Minimal renderable geometry built on the existing VAO/VBO/EBO wrappers.
// Layout matches the starter shader: location 0 = position (vec3),
// location 1 = color (vec3). RAII: GPU resources are released on destruction.
// Move-only; copy would double-free the GL objects.
class Mesh
{
public:
	Mesh(const GLfloat* vertices, GLsizeiptr verticesSize,
		 const GLuint* indices, GLsizeiptr indicesSize, GLsizei indexCount);
	~Mesh();

	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;
	Mesh(Mesh&& other) noexcept;
	Mesh& operator=(Mesh&& other) noexcept;

	void draw() const;

private:
	void release();

	VAO* m_vao = nullptr;
	VBO* m_vbo = nullptr;
	EBO* m_ebo = nullptr;
	GLsizei m_indexCount = 0;
};

#endif
