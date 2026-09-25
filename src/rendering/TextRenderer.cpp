#include "TextRenderer.h"

#include <algorithm>
#include <cstddef>
#include <iostream>

#include "BitmapFont.h"

bool TextRenderer::initialize()
{
	if (!m_program.load("shaders/hud.vert", "shaders/hud.frag"))
		return false;
	m_screenSizeLocation = m_program.uniform("screenSize");

	m_vao = std::make_unique<VAO>();
	m_vbo = std::make_unique<VBO>(nullptr, 0);
	m_vao->Bind();
	m_vao->LinkAttrib(*m_vbo, 0, 2, GL_FLOAT, sizeof(OverlayVertex),
		reinterpret_cast<const void*>(offsetof(OverlayVertex, position)));
	m_vao->LinkAttrib(*m_vbo, 1, 4, GL_FLOAT, sizeof(OverlayVertex),
		reinterpret_cast<const void*>(offsetof(OverlayVertex, color)));
	m_vao->Unbind();
	return true;
}

void TextRenderer::begin(int width, int height)
{
	m_width = width > 0 ? width : 1;
	m_height = height > 0 ? height : 1;
	m_vertices.clear();
}

void TextRenderer::addRect(float x, float y, float width, float height, const glm::vec4& color)
{
	const glm::vec2 a(x, y);
	const glm::vec2 b(x + width, y);
	const glm::vec2 c(x + width, y + height);
	const glm::vec2 d(x, y + height);
	m_vertices.push_back({ a, color });
	m_vertices.push_back({ d, color });
	m_vertices.push_back({ c, color });
	m_vertices.push_back({ a, color });
	m_vertices.push_back({ c, color });
	m_vertices.push_back({ b, color });
}

void TextRenderer::addText(float x, float y, const std::string& text, float pixelSize, const glm::vec4& color)
{
	float penX = x;
	float penY = y;
	for (char character : text)
	{
		if (character == '\n')
		{
			penX = x;
			penY += lineHeight(pixelSize);
			continue;
		}
		const BitmapFont::Glyph& glyph = BitmapFont::glyph(character);
		for (int row = 0; row < BitmapFont::kGlyphHeight; ++row)
		{
			for (int column = 0; column < BitmapFont::kGlyphWidth; ++column)
			{
				if ((glyph[row] >> (BitmapFont::kGlyphWidth - 1 - column)) & 1u)
					addRect(penX + column * pixelSize, penY + row * pixelSize, pixelSize, pixelSize, color);
			}
		}
		penX += (BitmapFont::kGlyphWidth + 1) * pixelSize;
	}
}

void TextRenderer::addShadowedText(float x, float y, const std::string& text, float pixelSize,
	const glm::vec4& color)
{
	addText(x + pixelSize * 0.7f, y + pixelSize * 0.7f, text, pixelSize, glm::vec4(0.0f, 0.0f, 0.0f, color.a * 0.85f));
	addText(x, y, text, pixelSize, color);
}

float TextRenderer::textWidth(const std::string& text, float pixelSize)
{
	std::size_t longest = 0;
	std::size_t current = 0;
	for (char character : text)
	{
		if (character == '\n')
		{
			current = 0;
			continue;
		}
		longest = std::max(longest, ++current);
	}
	return static_cast<float>(longest) * (BitmapFont::kGlyphWidth + 1) * pixelSize - pixelSize;
}

void TextRenderer::flush()
{
	if (!m_program.valid() || m_vertices.empty())
		return;

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_program.use();
	glUniform2f(m_screenSizeLocation, static_cast<float>(m_width), static_cast<float>(m_height));

	// Orphan-and-refill: one buffer object for the lifetime of the program.
	m_vbo->Bind();
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_vertices.size() * sizeof(OverlayVertex)),
		m_vertices.data(), GL_DYNAMIC_DRAW);
	m_vbo->Unbind();

	m_vao->Bind();
	glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_vertices.size()));
	m_vao->Unbind();

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	m_vertices.clear();
}
