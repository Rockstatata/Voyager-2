#include "SurfaceMaps.h"

#include <algorithm>
#include <cmath>

namespace
{
	float luminance(const std::vector<unsigned char>& rgba, int index)
	{
		return (0.2126f * rgba[index] + 0.7152f * rgba[index + 1] + 0.0722f * rgba[index + 2]) / 255.0f;
	}

	unsigned char encode(float component)
	{
		return static_cast<unsigned char>(std::clamp(component * 0.5f + 0.5f, 0.0f, 1.0f) * 255.0f + 0.5f);
	}
}

std::vector<unsigned char> SurfaceMaps::normalMapFromAlbedo(const std::vector<unsigned char>& albedo,
	int width, int height, float strength)
{
	std::vector<float> heights(static_cast<std::size_t>(width) * height);
	for (int i = 0; i < width * height; ++i)
		heights[i] = luminance(albedo, i * 4);

	auto heightAt = [&](int x, int y)
	{
		x = (x % width + width) % width;      // longitude wraps across the seam
		y = std::clamp(y, 0, height - 1);     // latitude stops at the poles
		return heights[static_cast<std::size_t>(y) * width + x];
	};

	// Sobel gradient: a 3x3 smoothed central difference, so JPEG noise does
	// not turn into sparkle.
	std::vector<unsigned char> normals(static_cast<std::size_t>(width) * height * 4);
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			const float dx =
				(heightAt(x + 1, y - 1) + 2.0f * heightAt(x + 1, y) + heightAt(x + 1, y + 1)) -
				(heightAt(x - 1, y - 1) + 2.0f * heightAt(x - 1, y) + heightAt(x - 1, y + 1));
			const float dy =
				(heightAt(x - 1, y + 1) + 2.0f * heightAt(x, y + 1) + heightAt(x + 1, y + 1)) -
				(heightAt(x - 1, y - 1) + 2.0f * heightAt(x, y - 1) + heightAt(x + 1, y - 1));
			float nx = -dx * strength;
			float ny = -dy * strength;
			float nz = 1.0f;
			const float length = std::sqrt(nx * nx + ny * ny + nz * nz);
			nx /= length;
			ny /= length;
			nz /= length;

			const std::size_t out = (static_cast<std::size_t>(y) * width + x) * 4;
			normals[out] = encode(nx);
			normals[out + 1] = encode(ny);
			normals[out + 2] = encode(nz);
			normals[out + 3] = 255;
		}
	}
	return normals;
}

std::vector<unsigned char> SurfaceMaps::oceanSpecularMap(const std::vector<unsigned char>& albedo,
	int width, int height)
{
	std::vector<unsigned char> mask(static_cast<std::size_t>(width) * height * 4);
	for (int i = 0; i < width * height; ++i)
	{
		const float r = albedo[i * 4] / 255.0f;
		const float g = albedo[i * 4 + 1] / 255.0f;
		const float b = albedo[i * 4 + 2] / 255.0f;
		const bool water = b > r * 1.25f && b > g * 1.02f && (r + g + b) < 1.5f;
		const unsigned char value = water ? 255 : 30;
		mask[i * 4] = value;
		mask[i * 4 + 1] = value;
		mask[i * 4 + 2] = value;
		mask[i * 4 + 3] = 255;
	}
	return mask;
}
