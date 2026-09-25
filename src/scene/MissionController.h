#ifndef MISSION_CONTROLLER_H
#define MISSION_CONTROLLER_H

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "BodyCatalog.h"
#include "MissionEphemeris.h"
#include "SimulationClock.h"

class CelestialBody;
class SceneObject;
class SolarSystem;
class Voyager2;

// Owns mission time and places everything that depends on it (bible R2):
// the one SimulationClock, the Horizons ephemeris, the binding from each
// planet to its table, Voyager's historical placement, bookmarks and the
// drawn trajectory. Knows nothing about cameras, input or rendering.
class MissionController
{
public:
	void load(SolarSystem& system, const glm::dvec3& sunPosition, std::vector<BookmarkData> bookmarks);
	void attachVoyager(Voyager2* voyager) { m_voyager = voyager; }
	// Gold path drawn from the same function the probe follows; returns it.
	SceneObject* buildTrajectory(SceneObject& group);

	// Advances the clock; then planets and (in Historical flight) Voyager.
	void update(double realDeltaSeconds, double speed);
	void placeBodies();
	void placeVoyager(bool snapHeading);

	// Jumps the date to bookmark `index` (0-based) and switches Voyager to
	// Historical flight. Returns the bookmark, or nullptr if out of range.
	const BookmarkData* jumpToBookmark(int index);
	void freezeBefore(const std::string& planetId, double daysBefore);

	// Encounter planet whose flyby region contains `position`, else nullptr.
	const CelestialBody* encounterPlanetNear(const glm::dvec3& position, double clearances) const;

	SimulationClock& clock() { return m_clock; }
	const SimulationClock& clock() const { return m_clock; }
	const MissionEphemeris& ephemeris() const { return m_ephemeris; }
	const std::vector<BookmarkData>& bookmarks() const { return m_bookmarks; }
	const glm::dvec3& sunPosition() const { return m_sunPosition; }

private:
	struct PlanetBinding
	{
		CelestialBody* body = nullptr;
		const MissionEphemeris::Planet* ephemeris = nullptr;
	};

	const MissionEphemeris::Encounter* findEncounter(const std::string& planetId) const;

	MissionEphemeris m_ephemeris;
	SimulationClock m_clock;
	std::vector<PlanetBinding> m_bindings;
	std::vector<BookmarkData> m_bookmarks;
	SolarSystem* m_system = nullptr;
	Voyager2* m_voyager = nullptr;
	glm::dvec3 m_sunPosition{ 0.0 };
};

#endif
