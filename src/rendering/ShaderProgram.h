#ifndef SHADER_PROGRAM_H
#define SHADER_PROGRAM_H

#include <string>
#include <unordered_map>

#include <glad/glad.h>

// RAII vertex+fragment program loaded from files under shaders/. Adds what
// the starter `Shader` class lacks: `#include "file.glsl"` (so the lighting
// model is written once and shared by the vertex stage for Gouraud and the
// fragment stage for Phong), compile/link error logging, and cached uniform
// locations. Move-only, like the other GPU owners.
class ShaderProgram
{
public:
	ShaderProgram() = default;
	~ShaderProgram();

	ShaderProgram(const ShaderProgram&) = delete;
	ShaderProgram& operator=(const ShaderProgram&) = delete;
	ShaderProgram(ShaderProgram&& other) noexcept;
	ShaderProgram& operator=(ShaderProgram&& other) noexcept;

	// Paths are relative to the working directory (the project root).
	bool load(const std::string& vertexPath, const std::string& fragmentPath);

	bool valid() const { return m_id != 0; }
	GLuint id() const { return m_id; }
	void use() const { glUseProgram(m_id); }

	// -1 when the uniform does not exist or was optimised away.
	GLint uniform(const std::string& name) const;

private:
	static bool readWithIncludes(const std::string& path, std::string& source, int depth);
	static GLuint compile(GLenum stage, const std::string& source, const std::string& path);
	void release();

	GLuint m_id = 0;
	mutable std::unordered_map<std::string, GLint> m_uniforms;
};

#endif
