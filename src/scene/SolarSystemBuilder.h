#ifndef SOLAR_SYSTEM_BUILDER_H
#define SOLAR_SYSTEM_BUILDER_H

#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "BodyCatalog.h"

class Mesh;
class SolarSystem;

// Turns catalog rows into scene objects: the Sun (with its glow shells),
// planets, moons on their parent-relative orbits, and every ring band.
// Every body shares the one sphere mesh (bible F9).
class SolarSystemBuilder
{
public:
	static void build(SolarSystem& system, const std::vector<CelestialBodyData>& bodies,
		const std::vector<RingBandData>& rings, const std::shared_ptr<Mesh>& sphere,
		const glm::dvec3& sunPosition);

	// Moons keep a visual orbital clock (planets follow the dated ephemeris).
	static constexpr double kMoonDaysPerSecond = 0.4;
};

#endif
