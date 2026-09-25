#include "LightingUniforms.h"

#include <algorithm>
#include <string>

#include "ShaderProgram.h"

namespace LightingUniforms
{
	void uploadLights(ShaderProgram& program, const LightingState& lighting, const glm::dvec3& origin)
	{
		glUniform1i(program.uniform("lightingEnabled"), lighting.enabled ? 1 : 0);
		glUniform1i(program.uniform("shadingTechnique"), static_cast<int>(lighting.technique));
		glUniform1f(program.uniform("ambientStrength"), lighting.ambient);
		glUniform1i(program.uniform("surfaceMapsEnabled"), lighting.surfaceMaps ? 1 : 0);

		for (int i = 0; i < kMaxLights; ++i)
		{
			const Light& light = lighting.lights[i];
			const std::string prefix = "lights[" + std::to_string(i) + "].";
			// Camera-relative, computed in double before narrowing.
			const glm::vec3 relativePosition(light.position - origin);
			const glm::vec3 radiance = light.color * light.intensity;
			const glm::vec3 direction = glm::length(light.direction) > 0.0f ? glm::normalize(light.direction)
				: glm::vec3(0.0f, -1.0f, 0.0f);
			glUniform1i(program.uniform(prefix + "type"), static_cast<int>(light.type));
			glUniform1i(program.uniform(prefix + "enabled"), light.enabled ? 1 : 0);
			glUniform3fv(program.uniform(prefix + "position"), 1, &relativePosition[0]);
			glUniform3fv(program.uniform(prefix + "direction"), 1, &direction[0]);
			glUniform3fv(program.uniform(prefix + "color"), 1, &radiance[0]);
			glUniform3fv(program.uniform(prefix + "attenuation"), 1, &light.attenuation[0]);
			glUniform1f(program.uniform(prefix + "innerCutoff"), light.innerCutoffCos);
			glUniform1f(program.uniform(prefix + "outerCutoff"), light.outerCutoffCos);
		}
	}

	void uploadTraceScene(ShaderProgram& program, const RayTraceScene& scene, ShadowMode shadows,
		const glm::dvec3& origin)
	{
		const int sphereCount = std::min(static_cast<int>(scene.spheres.size()), RayTraceScene::kMaxSpheres);
		glUniform1i(program.uniform("sphereCount"), sphereCount);
		for (int i = 0; i < sphereCount; ++i)
		{
			const TraceSphere& sphere = scene.spheres[i];
			const glm::vec4 packed(glm::vec3(sphere.center - origin), static_cast<float>(sphere.radius));
			const glm::vec3 material(sphere.specularStrength, sphere.specularPower, sphere.reflectivity);
			const std::string index = "[" + std::to_string(i) + "]";
			glUniform4fv(program.uniform("spheres" + index), 1, &packed[0]);
			glUniform1i(program.uniform("sphereEmissive" + index), sphere.emissive ? 1 : 0);
			// Only the ray-traced view declares these; -1 locations are ignored.
			glUniformMatrix3fv(program.uniform("sphereRotation" + index), 1, GL_FALSE, &sphere.worldToLocal[0][0]);
			glUniform1i(program.uniform("sphereLayer" + index), sphere.textureLayer);
			glUniform3fv(program.uniform("sphereMaterial" + index), 1, &material[0]);
		}

		const int ringCount = std::min(static_cast<int>(scene.rings.size()), RayTraceScene::kMaxRings);
		glUniform1i(program.uniform("ringCount"), ringCount);
		for (int i = 0; i < ringCount; ++i)
		{
			const TraceRing& ring = scene.rings[i];
			const glm::vec4 center(glm::vec3(ring.center - origin), static_cast<float>(ring.innerRadius));
			const glm::vec4 normal(ring.normal, static_cast<float>(ring.outerRadius));
			const std::string index = "[" + std::to_string(i) + "]";
			glUniform4fv(program.uniform("ringCenters" + index), 1, &center[0]);
			glUniform4fv(program.uniform("ringNormals" + index), 1, &normal[0]);
			glUniform1f(program.uniform("ringOpacity" + index), ring.opacity);
			glUniform3fv(program.uniform("ringColors" + index), 1, &ring.color[0]);
		}

		const glm::vec3 sunCenter(scene.sunPosition - origin);
		glUniform3fv(program.uniform("sunCenter"), 1, &sunCenter[0]);
		glUniform1f(program.uniform("sunLightRadius"), static_cast<float>(scene.sunLightRadius));
		glUniform1i(program.uniform("shadowMode"), static_cast<int>(shadows));
	}
}
