#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "ShaderProgram.h"
#include "../../VAO.h"
#include "../../VBO.h"

// Screen-space overlay for the HUD and body labels (bible section 39).
// Text is built from BitmapFont: every lit glyph cell becomes one small quad
// in pixel coordinates. The quads for a whole frame share ONE dynamic buffer
// and ONE draw call; the buffer is re-filled, never re-created (bible F8).
class TextRenderer
{
public:
	bool initialize();

	// Starts a frame with the framebuffer size in pixels (origin top-left).
	void begin(int width, int height);
	void addRect(float x, float y, float width, float height, const glm::vec4& color);
	// `pixelSize` is the edge of one glyph cell in screen pixels.
	void addText(float x, float y, const std::string& text, float pixelSize, const glm::vec4& color);
	// Text with a one-cell dark drop shadow, readable over bright planets.
	void addShadowedText(float x, float y, const std::string& text, float pixelSize, const glm::vec4& color);
	void flush();

	static float textWidth(const std::string& text, float pixelSize);
	static float lineHeight(float pixelSize) { return pixelSize * 9.0f; }

private:
	struct OverlayVertex
	{
		glm::vec2 position;
		glm::vec4 color;
	};

	ShaderProgram m_program;
	std::unique_ptr<VAO> m_vao;
	std::unique_ptr<VBO> m_vbo;
	std::vector<OverlayVertex> m_vertices;
	GLint m_screenSizeLocation = -1;
	int m_width = 1;
	int m_height = 1;
};

#endif
