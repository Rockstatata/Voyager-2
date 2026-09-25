#ifndef ENVIRONMENT_BUILDER_H
#define ENVIRONMENT_BUILDER_H

#include <memory>
#include <vector>

#include <glm/glm.hpp>

class Mesh;
class MissionEphemeris;
class Scene;
class SceneObject;

// Everything around the bodies: the camera-centred star layers, orbit guides,
// heliosphere boundaries, instanced small-body fields and the comet. Each
// category lands in its own named scene group (see Scene::group).
class EnvironmentBuilder
{
public:
	// Background layers are returned, not added to the scene: they are drawn
	// first, centred on the camera, without depth writes.
	static std::vector<std::unique_ptr<SceneObject>> buildStarLayers();

	static void buildOrbitGuides(SceneObject& group, const MissionEphemeris& ephemeris,
		const glm::dvec3& sunPosition);
	static void buildHeliosphere(SceneObject& group, const glm::dvec3& sunPosition);
	static void buildSmallBodyFields(SceneObject& group, const glm::dvec3& sunPosition);
	static void buildComet(SceneObject& group, const glm::dvec3& sunPosition, const std::shared_ptr<Mesh>& sphere);
};

#endif
