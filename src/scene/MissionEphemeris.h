#ifndef MISSION_EPHEMERIS_H
#define MISSION_EPHEMERIS_H

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "ScaleManager.h"
#include "Trajectory.h"

// Bible Phase 11: one synchronized ephemeris for the whole mission.
//
// Every planet and Voyager 2 are NASA/JPL Horizons state vectors in the same
// heliocentric ecliptic frame, sampled at the SAME Julian Date and mapped
// through the SAME ScaleManager distance law. Pure CPU/GLM, no OpenGL.
//
// Flyby clearance. The educational scale enlarges planets roughly 100x
// relative to their orbits, so Voyager's real 650,000 km Jupiter pass would
// render inside the planet. Near each world, Voyager's rendered offset g
// from the planet keeps its direction but its length becomes
//     L = |g| + (sqrt(|g|^2 + c^2) - |g|) * fade(|g| / c)
// where c is the encounter clearance (planet display radii scaled from the
// real closest-approach ratio). Far away the push fades to exactly zero; at
// closest approach Voyager sweeps round the planet at radius c, following
// the real hyperbolic flyby direction from the 10-minute Horizons samples.
class MissionEphemeris
{
public:
	struct Planet
	{
		std::string id;
		std::string displayName;
		double radiusKm = 0.0;
		double renderRadius = 0.0;
		Trajectory track;
	};

	struct Encounter
	{
		std::string planetId;
		std::string displayName;
		double closestApproachJulianDate = 0.0;
		double closestApproachKm = 0.0;       // from planet centre
		double clearanceRenderUnits = 0.0;    // rendered centre distance at closest approach
	};

	struct Telemetry
	{
		double sunDistanceAu = 0.0;
		double heliocentricSpeedKmPerSecond = 0.0;
		std::string nearestPlanet;
		double nearestPlanetDistanceKm = 0.0;
	};

	// Loads assets/trajectory/planets/<id>_heliocentric.csv for each planet.
	bool addPlanet(const std::string& id, const std::string& displayName,
		double radiusKm, double renderRadius);
	bool loadVoyager(const std::string& path);
	// Finds each closest approach in the dense Horizons windows. Call after
	// all planets and Voyager are loaded.
	void computeEncounters(const std::vector<std::string>& planetIds);

	void setSunPosition(const glm::dvec3& sunPosition) { m_sunPosition = sunPosition; }

	const Planet* findPlanet(const std::string& id) const;
	const std::vector<Planet>& planets() const { return m_planets; }
	const std::vector<Encounter>& encounters() const { return m_encounters; }
	const Trajectory& voyagerTrack() const { return m_voyager; }
	bool hasVoyager() const { return !m_voyager.empty(); }

	glm::dvec3 planetRenderPosition(const Planet& planet, double julianDate) const;
	glm::dvec3 voyagerRenderPosition(double julianDate) const;
	Telemetry voyagerTelemetry(double julianDate) const;

	// Full osculating two-body ellipse through the planet's state at
	// `epochJulianDate`, mapped to render space: the orbit guide the planet
	// visibly rides on (inclination and perihelion direction included).
	std::vector<glm::dvec3> orbitGuide(const Planet& planet, double epochJulianDate,
		unsigned int segments) const;

private:
	glm::dvec3 map(const glm::dvec3& heliocentricEclipticAu) const;

	ScaleManager m_scale;
	glm::dvec3 m_sunPosition{ 0.0 };
	std::vector<Planet> m_planets;
	Trajectory m_voyager;
	std::vector<Encounter> m_encounters;
	// Earth is not a flyby target, but launch starts inside its display radius.
	double m_earthClearance = 0.0;
};

#endif
