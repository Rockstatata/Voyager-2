#ifndef TEXTURE_2D_H
#define TEXTURE_2D_H

#include <string>
#include <vector>

#include <glad/glad.h>

class Texture2D
{
public:
	Texture2D() = default;
	~Texture2D();

	Texture2D(const Texture2D&) = delete;
	Texture2D& operator=(const Texture2D&) = delete;
	Texture2D(Texture2D&& other) noexcept;
	Texture2D& operator=(Texture2D&& other) noexcept;

	bool uploadRgba(int width, int height, const std::vector<unsigned char>& pixels,
					const std::string& debugName);

	// Decodes an image file on disk (jpg/png/etc, via stb_image) and uploads it.
	// Path is relative to the working directory, same rule as Shader (project root).
	bool loadFromFile(const std::string& path);

	void bind(GLuint textureUnit = 0) const;

	bool valid() const { return m_id != 0; }
	int width() const { return m_width; }
	int height() const { return m_height; }
	const std::string& debugName() const { return m_debugName; }

private:
	void release();

	GLuint m_id = 0;
	int m_width = 0;
	int m_height = 0;
	std::string m_debugName;
};

#endif
