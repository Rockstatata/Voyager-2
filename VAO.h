#ifndef VAO_CLASS_H
#define VAO_CLASS_H

#include <glad/glad.h>
#include "VBO.h"

// RAII wrapper around a GL vertex array object.
// Move-only, same reasoning as VBO.
class VAO {
public:
	GLuint ID = 0;

	VAO();
	~VAO();

	VAO(const VAO&) = delete;
	VAO& operator=(const VAO&) = delete;
	VAO(VAO&& other) noexcept;
	VAO& operator=(VAO&& other) noexcept;

	void LinkAttrib(const VBO& vbo, GLuint layout, GLuint numComponents, GLenum type, GLsizeiptr stride, void* offset);
	void Bind() const;
	void Unbind() const;

	// Releases the vertex array early. Idempotent.
	void Delete();
};

#endif
