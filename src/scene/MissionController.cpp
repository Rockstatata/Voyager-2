#include "MissionController.h"

#include <algorithm>
#include <cstdint>
#include <iostream>

#include "CelestialBody.h"
#include "SolarSystem.h"
#include "Voyager2.h"
#include "../rendering/MaterialLibrary.h"
#include "../rendering/Mesh.h"

void MissionController::load(SolarSystem& system, const glm::dvec3& sunPosition, std::vector<BookmarkData> bookmarks)
{
	m_system = &system;
	m_sunPosition = sunPosition;
	m_bookmarks = std::move(bookmarks);
	m_ephemeris.setSunPosition(sunPosition);

	for (CelestialBody* body : system.bodies())
	{
		const BodyType type = body->data().type;
		if (type == BodyType::Planet || type == BodyType::DwarfPlanet)
			m_ephemeris.addPlanet(body->data().id, body->data().displayName, body->data().radiusKm,
				body->transform().scale.x);
	}
	for (CelestialBody* body : system.bodies())
	{
		if (const MissionEphemeris::Planet* planet = m_ephemeris.findPlanet(body->data().id))
			m_bindings.push_back({ body, planet });
	}

	if (m_ephemeris.loadVoyager("assets/trajectory/voyager2_heliocentric.csv"))
	{
		m_ephemeris.computeEncounters({ "jupiter", "saturn", "uranus", "neptune" });
		const Trajectory& track = m_ephemeris.voyagerTrack();
		m_clock.setRange(track.startJulianDate(), track.endJulianDate());
		std::vector<double> encounterDates;
		for (const MissionEphemeris::Encounter& encounter : m_ephemeris.encounters())
			encounterDates.push_back(encounter.closestApproachJulianDate);
		m_clock.setEncounters(std::move(encounterDates));
		m_clock.setJulianDate(track.startJulianDate());
	}
	else
	{
		// Without the Voyager table the planets still run on their own data.
		m_clock.setRange(2443376.5, 2462503.5);
		m_clock.setJulianDate(2443376.5);
	}

	std::cout << "[TRAJECTORY] synchronized ephemeris: " << m_bindings.size()
			  << " planets + Voyager 2 on one simulation date" << std::endl;
}

SceneObject* MissionController::buildTrajectory(SceneObject& group)
{
	if (!m_ephemeris.hasVoyager())
		return nullptr;

	// Every Horizons row (1-minute spacing at each closest approach) through
	// the same voyagerRenderPosition the probe uses, stored Sun-relative.
	const auto& samples = m_ephemeris.voyagerTrack().samples();
	MeshData data;
	data.vertices.reserve(samples.size());
	data.indices.reserve(samples.size());
	for (std::size_t i = 0; i < samples.size(); ++i)
	{
		Vertex vertex;
		vertex.position = glm::vec3(m_ephemeris.voyagerRenderPosition(samples[i].julianDate) - m_sunPosition);
		vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
		vertex.texCoord = glm::vec2(static_cast<float>(i) / static_cast<float>(samples.size() - 1), 0.0f);
		data.vertices.push_back(vertex);
		data.indices.push_back(static_cast<std::uint32_t>(i));
	}

	auto path = std::make_unique<SceneObject>("voyager2_historical_trajectory");
	path->setMesh(std::make_shared<Mesh>(data, PrimitiveMode::LineStrip));
	path->setMaterial(MaterialLibrary::flat(glm::vec3(0.95f, 0.72f, 0.30f)));
	path->transform().position = m_sunPosition;
	return &group.addChild(std::move(path));
}

void MissionController::update(double realDeltaSeconds, double speed)
{
	m_clock.setSpeed(speed);
	m_clock.update(realDeltaSeconds);
	placeBodies();
	if (m_voyager != nullptr && m_voyager->flightMode() == Voyager2::FlightMode::Historical)
		placeVoyager(false);
}

void MissionController::placeBodies()
{
	const double julianDate = m_clock.julianDate();
	for (const PlanetBinding& binding : m_bindings)
		binding.body->transform().position = m_ephemeris.planetRenderPosition(*binding.ephemeris, julianDate);
}

void MissionController::placeVoyager(bool snapHeading)
{
	if (m_voyager == nullptr || !m_ephemeris.hasVoyager())
		return;

	const double julianDate = m_clock.julianDate();
	// Heading from a short central difference on the rendered path; the step
	// shrinks with the encounter slow-motion so the flyby turn is resolved.
	const double step = std::clamp(0.5 * m_clock.encounterFactor(), 0.002, 0.5);
	const glm::dvec3 before = m_ephemeris.voyagerRenderPosition(julianDate - step);
	const glm::dvec3 after = m_ephemeris.voyagerRenderPosition(julianDate + step);
	const double renderSpeed = glm::length(after - before) / (2.0 * step) * m_clock.daysPerSecond();
	m_voyager->setHistoricalState(m_ephemeris.voyagerRenderPosition(julianDate), after - before,
		renderSpeed, snapHeading);
}

const MissionEphemeris::Encounter* MissionController::findEncounter(const std::string& planetId) const
{
	for (const MissionEphemeris::Encounter& encounter : m_ephemeris.encounters())
	{
		if (encounter.planetId == planetId)
			return &encounter;
	}
	return nullptr;
}

const BookmarkData* MissionController::jumpToBookmark(int index)
{
	if (index < 0 || index >= static_cast<int>(m_bookmarks.size()))
		return nullptr;

	const BookmarkData& bookmark = m_bookmarks[index];
	double julianDate = bookmark.julianDate > 0.0 ? bookmark.julianDate : m_clock.startJulianDate();
	if (const MissionEphemeris::Encounter* encounter = findEncounter(bookmark.planetId))
		julianDate = encounter->closestApproachJulianDate - bookmark.leadDays;

	if (m_voyager != nullptr)
		m_voyager->setFlightMode(Voyager2::FlightMode::Historical);
	m_clock.setJulianDate(julianDate);
	m_clock.setPaused(false);
	placeBodies();
	placeVoyager(true);
	std::cout << "[TRAJECTORY] bookmark: " << bookmark.name << std::endl;
	return &bookmark;
}

void MissionController::freezeBefore(const std::string& planetId, double daysBefore)
{
	if (const MissionEphemeris::Encounter* encounter = findEncounter(planetId))
		m_clock.setJulianDate(encounter->closestApproachJulianDate - daysBefore);
	m_clock.setPaused(true);
	placeBodies();
	placeVoyager(true);
}

const CelestialBody* MissionController::encounterPlanetNear(const glm::dvec3& position, double clearances) const
{
	if (m_system == nullptr)
		return nullptr;
	for (const MissionEphemeris::Encounter& encounter : m_ephemeris.encounters())
	{
		const CelestialBody* planet = m_system->find(encounter.planetId);
		if (planet != nullptr &&
			glm::length(planet->transform().position - position) < encounter.clearanceRenderUnits * clearances)
			return planet;
	}
	return nullptr;
}
