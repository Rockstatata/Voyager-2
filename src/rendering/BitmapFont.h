#ifndef BITMAP_FONT_H
#define BITMAP_FONT_H

#include <array>
#include <cstdint>

// Project-authored 5x7 pixel font for the HUD and labels. Each glyph is seven
// rows of five cells written as text ('#' = lit), so every character can be
// read and checked directly in this file. Lower-case input is drawn with the
// upper-case glyph. No font file, atlas or third-party glyph data is used.
class BitmapFont
{
public:
	static constexpr int kGlyphWidth = 5;
	static constexpr int kGlyphHeight = 7;

	// Row bitmasks, top row first; bit 4 is the leftmost cell.
	using Glyph = std::array<std::uint8_t, kGlyphHeight>;

	static const Glyph& glyph(char character);
};

#endif
