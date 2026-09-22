#include "EBO.h"

#include <utility>

EBO::EBO(const void* indices, GLsizeiptr size) {
	glGenBuffers(1, &ID);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
}

EBO::~EBO() {
	Delete();
}

EBO::EBO(EBO&& other) noexcept : ID(std::exchange(other.ID, 0)) {
}

EBO& EBO::operator=(EBO&& other) noexcept {
	if (this != &other) {
		Delete();
		ID = std::exchange(other.ID, 0);
	}
	return *this;
}

void EBO::Bind() const {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
}
void EBO::Unbind() const {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
void EBO::Delete() {
	if (ID != 0) {
		glDeleteBuffers(1, &ID);
		ID = 0;
	}
}
