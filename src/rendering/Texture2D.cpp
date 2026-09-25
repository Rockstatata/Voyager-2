#include "Texture2D.h"

#include <iostream>
#include <limits>
#include <utility>

#include <stb/stb_image.h>

Texture2D::~Texture2D()
{
	release();
}

Texture2D::Texture2D(Texture2D&& other) noexcept
	: m_id(std::exchange(other.m_id, 0)),
	  m_width(std::exchange(other.m_width, 0)),
	  m_height(std::exchange(other.m_height, 0)),
	  m_debugName(std::move(other.m_debugName))
{
}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept
{
	if (this != &other)
	{
		release();
		m_id = std::exchange(other.m_id, 0);
		m_width = std::exchange(other.m_width, 0);
		m_height = std::exchange(other.m_height, 0);
		m_debugName = std::move(other.m_debugName);
	}
	return *this;
}

bool Texture2D::uploadRgba(int width, int height,
					   const std::vector<unsigned char>& pixels,
					   const std::string& debugName)
{
	release();
	const std::size_t expectedBytes =
		static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
	if (width <= 0 || height <= 0 || pixels.size() != expectedBytes ||
		width > std::numeric_limits<GLsizei>::max() ||
		height > std::numeric_limits<GLsizei>::max())
	{
		std::cout << "[ASSET] generated texture rejected: " << debugName << std::endl;
		return false;
	}
	m_width = width;
	m_height = height;

	glGenTextures(1, &m_id);
	glBindTexture(GL_TEXTURE_2D, m_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0,
				 GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	glGenerateMipmap(GL_TEXTURE_2D);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glBindTexture(GL_TEXTURE_2D, 0);

	m_debugName = debugName;
	std::cout << "[ASSET] texture uploaded: " << debugName << " ("
			  << m_width << "x" << m_height << ")" << std::endl;
	return true;
}

bool Texture2D::loadFromFile(const std::string& path)
{
	int width = 0;
	int height = 0;
	std::vector<unsigned char> pixels;
	if (!decodeFile(path, width, height, pixels))
		return false;
	return uploadRgba(width, height, pixels, path);
}

bool Texture2D::decodeFile(const std::string& path, int& width, int& height, std::vector<unsigned char>& pixels)
{
	// Real photo maps are stored top-row-first in the file but OpenGL's (0,0)
	// texel is the bottom-left, so flip on load to keep our UV convention
	// (v=0 at the sphere's north pole row) matching the pixel rows.
	stbi_set_flip_vertically_on_load(true);

	int sourceChannels = 0;
	unsigned char* decoded = stbi_load(path.c_str(), &width, &height, &sourceChannels, 4);
	if (decoded == nullptr)
	{
		std::cout << "[ASSET] failed to load texture file: " << path
				  << " (" << stbi_failure_reason() << ")" << std::endl;
		return false;
	}

	pixels.assign(decoded, decoded + (static_cast<std::size_t>(width) * height * 4));
	stbi_image_free(decoded);
	return true;
}

void Texture2D::bind(GLuint textureUnit) const
{
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture2D::release()
{
	if (m_id != 0)
	{
		glDeleteTextures(1, &m_id);
		m_id = 0;
	}
	m_width = 0;
	m_height = 0;
	m_debugName.clear();
}
