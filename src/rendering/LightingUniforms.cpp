#include "LightingUniforms.h"

#include <algorithm>
#include <array>
#include <string>
#include <unordered_map>
#include <vector>

#include "ShaderProgram.h"

namespace
{
	// "prefix[i]suffix" for i in [0, count), built once per name: the per-frame
	// uploads then only look the cached string up instead of formatting it.
	const std::vector<std::string>& indexedNames(const std::string& prefix, const std::string& suffix, int count)
	{
		static std::unordered_map<std::string, std::vector<std::string>> cache;
		std::vector<std::string>& names = cache[prefix + "#" + suffix];
		if (names.empty())
		{
			for (int i = 0; i < count; ++i)
				names.push_back(prefix + "[" + std::to_string(i) + "]" + suffix);
		}
		return names;
	}
}

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
			// Camera-relative, computed in double before narrowing.
			const glm::vec3 relativePosition(light.position - origin);
			const glm::vec3 radiance = light.color * light.intensity;
			const glm::vec3 direction = glm::length(light.direction) > 0.0f ? glm::normalize(light.direction)
				: glm::vec3(0.0f, -1.0f, 0.0f);
			glUniform1i(program.uniform(indexedNames("lights", ".type", kMaxLights)[i]), static_cast<int>(light.type));
			glUniform1i(program.uniform(indexedNames("lights", ".enabled", kMaxLights)[i]), light.enabled ? 1 : 0);
			glUniform3fv(program.uniform(indexedNames("lights", ".position", kMaxLights)[i]), 1, &relativePosition[0]);
			glUniform3fv(program.uniform(indexedNames("lights", ".direction", kMaxLights)[i]), 1, &direction[0]);
			glUniform3fv(program.uniform(indexedNames("lights", ".color", kMaxLights)[i]), 1, &radiance[0]);
			glUniform3fv(program.uniform(indexedNames("lights", ".attenuation", kMaxLights)[i]), 1, &light.attenuation[0]);
			glUniform1f(program.uniform(indexedNames("lights", ".innerCutoff", kMaxLights)[i]), light.innerCutoffCos);
			glUniform1f(program.uniform(indexedNames("lights", ".outerCutoff", kMaxLights)[i]), light.outerCutoffCos);
		}
	}

	void uploadTraceScene(ShaderProgram& program, const RayTraceScene& scene, ShadowMode shadows,
		const glm::dvec3& origin)
	{
		// Arrays of basic types have consecutive uniform locations, so each
		// array is packed on the CPU and sent in ONE call from element [0]
		// (instead of one name lookup and call per element).
		const int sphereCount = std::min(static_cast<int>(scene.spheres.size()), RayTraceScene::kMaxSpheres);
		std::array<glm::vec4, RayTraceScene::kMaxSpheres> spheres{};
		std::array<GLint, RayTraceScene::kMaxSpheres> emissive{};
		std::array<glm::mat3, RayTraceScene::kMaxSpheres> rotations{};
		std::array<GLint, RayTraceScene::kMaxSpheres> layers{};
		std::array<glm::vec3, RayTraceScene::kMaxSpheres> materials{};
		for (int i = 0; i < sphereCount; ++i)
		{
			const TraceSphere& sphere = scene.spheres[i];
			spheres[i] = glm::vec4(glm::vec3(sphere.center - origin), static_cast<float>(sphere.radius));
			emissive[i] = sphere.emissive ? 1 : 0;
			rotations[i] = sphere.worldToLocal;
			layers[i] = sphere.textureLayer;
			materials[i] = glm::vec3(sphere.specularStrength, sphere.specularPower, sphere.reflectivity);
		}
		glUniform1i(program.uniform("sphereCount"), sphereCount);
		if (sphereCount > 0)
		{
			glUniform4fv(program.uniform("spheres[0]"), sphereCount, &spheres[0][0]);
			glUniform1iv(program.uniform("sphereEmissive[0]"), sphereCount, emissive.data());
			// Only the ray-traced view declares these; -1 locations are ignored.
			glUniformMatrix3fv(program.uniform("sphereRotation[0]"), sphereCount, GL_FALSE, &rotations[0][0][0]);
			glUniform1iv(program.uniform("sphereLayer[0]"), sphereCount, layers.data());
			glUniform3fv(program.uniform("sphereMaterial[0]"), sphereCount, &materials[0][0]);
		}

		const int ringCount = std::min(static_cast<int>(scene.rings.size()), RayTraceScene::kMaxRings);
		std::array<glm::vec4, RayTraceScene::kMaxRings> centers{};
		std::array<glm::vec4, RayTraceScene::kMaxRings> normals{};
		std::array<float, RayTraceScene::kMaxRings> opacities{};
		std::array<glm::vec3, RayTraceScene::kMaxRings> colors{};
		for (int i = 0; i < ringCount; ++i)
		{
			const TraceRing& ring = scene.rings[i];
			centers[i] = glm::vec4(glm::vec3(ring.center - origin), static_cast<float>(ring.innerRadius));
			normals[i] = glm::vec4(ring.normal, static_cast<float>(ring.outerRadius));
			opacities[i] = ring.opacity;
			colors[i] = ring.color;
		}
		glUniform1i(program.uniform("ringCount"), ringCount);
		if (ringCount > 0)
		{
			glUniform4fv(program.uniform("ringCenters[0]"), ringCount, &centers[0][0]);
			glUniform4fv(program.uniform("ringNormals[0]"), ringCount, &normals[0][0]);
			glUniform1fv(program.uniform("ringOpacity[0]"), ringCount, opacities.data());
			glUniform3fv(program.uniform("ringColors[0]"), ringCount, &colors[0][0]);
		}

		const glm::vec3 sunCenter(scene.sunPosition - origin);
		glUniform3fv(program.uniform("sunCenter"), 1, &sunCenter[0]);
		glUniform1f(program.uniform("sunLightRadius"), static_cast<float>(scene.sunLightRadius));
		glUniform1i(program.uniform("shadowMode"), static_cast<int>(shadows));
	}
}
