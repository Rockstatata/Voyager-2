#include "RayTracer.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "Camera.h"
#include "LightingUniforms.h"
#include "Texture2D.h"

RayTracer::~RayTracer()
{
	if (m_atlas != 0)
		glDeleteTextures(1, &m_atlas);
	if (m_emptyVao != 0)
		glDeleteVertexArrays(1, &m_emptyVao);
}

bool RayTracer::initialize()
{
	if (!m_program.load("shaders/raytrace.vert", "shaders/raytrace.frag"))
		return false;
	// Core profile needs a bound VAO even for a draw with no attributes.
	glGenVertexArrays(1, &m_emptyVao);
	return true;
}

void RayTracer::buildAtlas()
{
	m_atlasBuilt = true;
	const int layers = std::max(static_cast<int>(m_texturePaths.size()), 1);
	glGenTextures(1, &m_atlas);
	glBindTexture(GL_TEXTURE_2D_ARRAY, m_atlas);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, kAtlasWidth, kAtlasHeight, layers, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	// Every map is resampled (bilinear) to one layer size, because a texture
	// array needs identical layers. 1024x512 keeps each world recognisable.
	std::vector<unsigned char> layer(static_cast<std::size_t>(kAtlasWidth) * kAtlasHeight * 4);
	for (int index = 0; index < static_cast<int>(m_texturePaths.size()); ++index)
	{
		int width = 0;
		int height = 0;
		std::vector<unsigned char> source;
		if (!Texture2D::decodeFile(m_texturePaths[index], width, height, source))
		{
			std::fill(layer.begin(), layer.end(), static_cast<unsigned char>(150));
		}
		else
		{
			for (int y = 0; y < kAtlasHeight; ++y)
			{
				for (int x = 0; x < kAtlasWidth; ++x)
				{
					const float sx = (x + 0.5f) * width / kAtlasWidth - 0.5f;
					const float sy = (y + 0.5f) * height / kAtlasHeight - 0.5f;
					const int x0 = std::clamp(static_cast<int>(std::floor(sx)), 0, width - 1);
					const int y0 = std::clamp(static_cast<int>(std::floor(sy)), 0, height - 1);
					const int x1 = std::min(x0 + 1, width - 1);
					const int y1 = std::min(y0 + 1, height - 1);
					const float fx = std::clamp(sx - x0, 0.0f, 1.0f);
					const float fy = std::clamp(sy - y0, 0.0f, 1.0f);
					for (int c = 0; c < 4; ++c)
					{
						auto at = [&](int px, int py) { return static_cast<float>(source[(static_cast<std::size_t>(py) * width + px) * 4 + c]); };
						const float top = at(x0, y0) * (1.0f - fx) + at(x1, y0) * fx;
						const float bottom = at(x0, y1) * (1.0f - fx) + at(x1, y1) * fx;
						layer[(static_cast<std::size_t>(y) * kAtlasWidth + x) * 4 + c] =
							static_cast<unsigned char>(top * (1.0f - fy) + bottom * fy + 0.5f);
					}
				}
			}
		}
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, index, kAtlasWidth, kAtlasHeight, 1,
			GL_RGBA, GL_UNSIGNED_BYTE, layer.data());
	}
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	std::cout << "[SHADER] ray-tracer albedo atlas: " << m_texturePaths.size() << " layers of "
			  << kAtlasWidth << "x" << kAtlasHeight << std::endl;
}

void RayTracer::render(const Camera& camera, float aspectRatio, const RayTraceScene& scene,
	const LightingState& lighting, float farPlane)
{
	if (!m_program.valid())
		return;
	if (!m_atlasBuilt)
		buildAtlas();

	m_program.use();
	const glm::dvec3 origin = camera.position();
	const glm::vec3 forward = camera.forward();
	const glm::vec3 right = camera.right();
	const glm::vec3 up = camera.up();
	glUniform3fv(m_program.uniform("cameraForward"), 1, &forward[0]);
	glUniform3fv(m_program.uniform("cameraRight"), 1, &right[0]);
	glUniform3fv(m_program.uniform("cameraUp"), 1, &up[0]);
	glUniform1f(m_program.uniform("tanHalfFov"), std::tan(glm::radians(camera.fieldOfView()) * 0.5f));
	glUniform1f(m_program.uniform("aspectRatio"), aspectRatio);
	glUniform1f(m_program.uniform("logDepthCoefficient"), 2.0f / std::log2(farPlane + 1.0f));
	glUniform1i(m_program.uniform("maxBounces"), m_maxBounces);
	glUniform1i(m_program.uniform("albedoAtlas"), 0);
	LightingUniforms::uploadLights(m_program, lighting, origin);
	// The traced view always keeps its shadow rays unless shadows are off.
	LightingUniforms::uploadTraceScene(m_program, scene,
		lighting.enabled ? lighting.shadows : ShadowMode::Off, origin);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, m_atlas);

	// Depth-tested so nearer raster geometry (Voyager) stays in front; the
	// shader writes its own depth for each hit, and blends translucent rings.
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindVertexArray(m_emptyVao);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}
