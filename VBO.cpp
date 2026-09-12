#include "VBO.h"

#include <utility>

VBO::VBO(const GLfloat* vertices, GLsizeiptr size) {
	glGenBuffers(1, &ID);

	glBindBuffer(GL_ARRAY_BUFFER, ID);
	glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
}

VBO::~VBO() {
	Delete();
}

VBO::VBO(VBO&& other) noexcept : ID(std::exchange(other.ID, 0)) {
}

VBO& VBO::operator=(VBO&& other) noexcept {
	if (this != &other) {
		Delete();
		ID = std::exchange(other.ID, 0);
	}
	return *this;
}

void VBO::Bind() const {
	glBindBuffer(GL_ARRAY_BUFFER, ID);
}
void VBO::Unbind() const {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
void VBO::Delete() {
	if (ID != 0) {
		glDeleteBuffers(1, &ID);
		ID = 0;
	}
}
