#ifndef LIGHTING_UNIFORMS_H
#define LIGHTING_UNIFORMS_H

#include <glm/glm.hpp>

#include "Lighting.h"
#include "RayTraceScene.h"

class ShaderProgram;

// Uploads the uniforms declared by shaders/lighting.glsl and
// shaders/raytrace.glsl. Shared by the raster Renderer and the RayTracer so
// both see exactly the same lights and shadow geometry. `origin` is the
// camera position: everything is converted to camera-relative floats.
namespace LightingUniforms
{
	void uploadLights(ShaderProgram& program, const LightingState& lighting, const glm::dvec3& origin);
	void uploadTraceScene(ShaderProgram& program, const RayTraceScene& scene, ShadowMode shadows,
		const glm::dvec3& origin);
}

#endif
