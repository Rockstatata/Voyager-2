#include "ShaderProgram.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

ShaderProgram::~ShaderProgram()
{
	release();
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
	: m_id(std::exchange(other.m_id, 0)), m_uniforms(std::move(other.m_uniforms))
{
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept
{
	if (this != &other)
	{
		release();
		m_id = std::exchange(other.m_id, 0);
		m_uniforms = std::move(other.m_uniforms);
	}
	return *this;
}

void ShaderProgram::release()
{
	if (m_id != 0)
	{
		glDeleteProgram(m_id);
		m_id = 0;
	}
	m_uniforms.clear();
}

bool ShaderProgram::readWithIncludes(const std::string& path, std::string& source, int depth)
{
	if (depth > 8)
	{
		std::cout << "[SHADER] include nesting too deep at " << path << std::endl;
		return false;
	}
	std::ifstream file(path);
	if (!file)
	{
		std::cout << "[SHADER] cannot read " << path
				  << " (the working directory must be the project root)" << std::endl;
		return false;
	}

	const std::filesystem::path directory = std::filesystem::path(path).parent_path();
	std::string line;
	while (std::getline(file, line))
	{
		const std::size_t directive = line.find("#include");
		if (directive != std::string::npos && line.find_first_not_of(" \t") == directive)
		{
			const std::size_t open = line.find('"', directive);
			const std::size_t close = line.find('"', open + 1);
			if (open == std::string::npos || close == std::string::npos)
			{
				std::cout << "[SHADER] malformed #include in " << path << ": " << line << std::endl;
				return false;
			}
			const std::string included = (directory / line.substr(open + 1, close - open - 1)).generic_string();
			if (!readWithIncludes(included, source, depth + 1))
				return false;
			continue;
		}
		source += line;
		source += '\n';
	}
	return true;
}

GLuint ShaderProgram::compile(GLenum stage, const std::string& source, const std::string& path)
{
	const GLuint shader = glCreateShader(stage);
	const char* text = source.c_str();
	glShaderSource(shader, 1, &text, nullptr);
	glCompileShader(shader);

	GLint compiled = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (compiled == GL_FALSE)
	{
		char log[2048] = {};
		glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
		std::cout << "[SHADER] compile failed: " << path << "\n" << log << std::endl;
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

bool ShaderProgram::load(const std::string& vertexPath, const std::string& fragmentPath)
{
	release();
	std::string vertexSource;
	std::string fragmentSource;
	if (!readWithIncludes(vertexPath, vertexSource, 0) || !readWithIncludes(fragmentPath, fragmentSource, 0))
		return false;

	const GLuint vertex = compile(GL_VERTEX_SHADER, vertexSource, vertexPath);
	const GLuint fragment = compile(GL_FRAGMENT_SHADER, fragmentSource, fragmentPath);
	if (vertex == 0 || fragment == 0)
	{
		glDeleteShader(vertex);
		glDeleteShader(fragment);
		return false;
	}

	m_id = glCreateProgram();
	glAttachShader(m_id, vertex);
	glAttachShader(m_id, fragment);
	glLinkProgram(m_id);
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	GLint linked = GL_FALSE;
	glGetProgramiv(m_id, GL_LINK_STATUS, &linked);
	if (linked == GL_FALSE)
	{
		char log[2048] = {};
		glGetProgramInfoLog(m_id, sizeof(log), nullptr, log);
		std::cout << "[SHADER] link failed: " << vertexPath << " + " << fragmentPath << "\n" << log << std::endl;
		release();
		return false;
	}
	std::cout << "[SHADER] loaded " << vertexPath << " + " << fragmentPath << std::endl;
	return true;
}

GLint ShaderProgram::uniform(const std::string& name) const
{
	const auto found = m_uniforms.find(name);
	if (found != m_uniforms.end())
		return found->second;
	const GLint location = glGetUniformLocation(m_id, name.c_str());
	m_uniforms.emplace(name, location);
	return location;
}
