#ifndef VBO_CLASS_H
#define VBO_CLASS_H

#include <glad/glad.h>

// RAII wrapper around a GL array buffer.
// Move-only: copying would let two objects delete the same buffer name.
class VBO {
public:
	GLuint ID = 0;

	VBO(const void* data, GLsizeiptr size);
	~VBO();

	VBO(const VBO&) = delete;
	VBO& operator=(const VBO&) = delete;
	VBO(VBO&& other) noexcept;
	VBO& operator=(VBO&& other) noexcept;

	void Bind() const;
	void Unbind() const;

	// Releases the buffer early. Idempotent, so the destructor stays safe.
	void Delete();
};

#endif
