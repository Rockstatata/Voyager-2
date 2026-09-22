#ifndef EBO_CLASS_H
#define EBO_CLASS_H

#include <glad/glad.h>

// RAII wrapper around a GL element (index) buffer.
// Move-only, same reasoning as VBO.
class EBO {
public:
	GLuint ID = 0;

	EBO(const void* indices, GLsizeiptr size);
	~EBO();

	EBO(const EBO&) = delete;
	EBO& operator=(const EBO&) = delete;
	EBO(EBO&& other) noexcept;
	EBO& operator=(EBO&& other) noexcept;

	void Bind() const;
	void Unbind() const;

	// Releases the buffer early. Idempotent.
	void Delete();
};

#endif
