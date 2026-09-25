#include "MissionEphemeris.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

#include <glm/gtc/constants.hpp>

namespace
{
	// Heliocentric gravitational parameter, AU^3/day^2 (Gaussian constant k^2).
	constexpr double kSunGravitationalParameter = 2.9591220828559115e-4;
	constexpr double kSecondsPerDay = 86400.0;

	double smoothStep(double edge0, double edge1, double x)
	{
		const double t = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
		return t * t * (3.0 - 2.0 * t);
	}

	// Pushes `point` radially away from `centre` so it never renders closer
	// than `clearance`, with the correction fading to zero by 8 clearances.
	glm::dvec3 applyClearance(const glm::dvec3& point, const glm::dvec3& centre, double clearance,
		const glm::dvec3& fallbackDirection)
	{
		const glm::dvec3 offset = point - centre;
		const double length = glm::length(offset);
		if (clearance <= 0.0 || length >= 8.0 * clearance)
			return point;

		const double fade = 1.0 - smoothStep(4.0 * clearance, 8.0 * clearance, length);
		const double pushed = length + (std::sqrt(length * length + clearance * clearance) - length) * fade;
		const glm::dvec3 direction = length > 1e-12 ? offset / length : fallbackDirection;
		return centre + direction * pushed;
	}
}

bool MissionEphemeris::addPlanet(const std::string& id, const std::string& displayName,
	double radiusKm, double renderRadius)
{
	Planet planet;
	planet.id = id;
	planet.displayName = displayName;
	planet.radiusKm = radiusKm;
	planet.renderRadius = renderRadius;
	if (!planet.track.loadCsv("assets/trajectory/planets/" + id + "_heliocentric.csv"))
		return false;

	if (id == "earth")
		m_earthClearance = renderRadius * 3.0;
	m_planets.push_back(std::move(planet));
	return true;
}

bool MissionEphemeris::loadVoyager(const std::string& path)
{
	return m_voyager.loadCsv(path);
}

const MissionEphemeris::Planet* MissionEphemeris::findPlanet(const std::string& id) const
{
	const auto it = std::find_if(m_planets.begin(), m_planets.end(),
		[&id](const Planet& planet) { return planet.id == id; });
	return it != m_planets.end() ? &*it : nullptr;
}

void MissionEphemeris::computeEncounters(const std::vector<std::string>& planetIds)
{
	m_encounters.clear();
	if (m_voyager.empty())
		return;

	for (const std::string& id : planetIds)
	{
		const Planet* planet = findPlanet(id);
		if (planet == nullptr)
			continue;

		auto separationAu = [&](double julianDate)
		{
			return glm::length(m_voyager.heliocentricPositionAtJulianDate(julianDate) -
				planet->track.heliocentricPositionAtJulianDate(julianDate));
		};

		// Coarse: every Voyager row (10-minute spacing inside the windows).
		double bestDate = m_voyager.startJulianDate();
		double bestDistance = separationAu(bestDate);
		for (const TrajectorySample& sample : m_voyager.samples())
		{
			const double distance = separationAu(sample.julianDate);
			if (distance < bestDistance)
			{
				bestDistance = distance;
				bestDate = sample.julianDate;
			}
		}

		// Fine: golden-section search on the Hermite curves, +/- 30 minutes.
		double low = bestDate - 0.02;
		double high = bestDate + 0.02;
		const double ratio = 0.5 * (std::sqrt(5.0) - 1.0);
		for (int iteration = 0; iteration < 60; ++iteration)
		{
			const double a = high - ratio * (high - low);
			const double b = low + ratio * (high - low);
			if (separationAu(a) < separationAu(b))
				high = b;
			else
				low = a;
		}

		Encounter encounter;
		encounter.planetId = planet->id;
		encounter.displayName = planet->displayName;
		encounter.closestApproachJulianDate = 0.5 * (low + high);
		encounter.closestApproachKm = separationAu(encounter.closestApproachJulianDate) *
			ScaleManager::kKilometresPerAu;
		// Real ratio (Neptune 1.2, Saturn 2.8, Uranus 4.2, Jupiter 9.3 radii)
		// compressed as 1.5 + sqrt(ratio): the ORDER of how closely Voyager
		// grazed each world survives, and every pass clears rings and surface.
		encounter.clearanceRenderUnits = planet->renderRadius *
			(1.5 + std::sqrt(encounter.closestApproachKm / planet->radiusKm));
		m_encounters.push_back(encounter);

		std::cout << "[TRAJECTORY] " << planet->displayName << " closest approach JD "
			<< std::fixed << std::setprecision(4) << encounter.closestApproachJulianDate
			<< ", " << std::setprecision(0) << encounter.closestApproachKm << " km ("
			<< std::setprecision(2) << encounter.closestApproachKm / planet->radiusKm
			<< " radii), display clearance " << encounter.clearanceRenderUnits / planet->renderRadius
			<< " display radii" << std::endl;
	}
}

glm::dvec3 MissionEphemeris::map(const glm::dvec3& heliocentricEclipticAu) const
{
	return Trajectory::mapHeliocentricToRender(Trajectory::eclipticToScene(heliocentricEclipticAu),
		m_scale, m_sunPosition);
}

glm::dvec3 MissionEphemeris::planetRenderPosition(const Planet& planet, double julianDate) const
{
	return map(planet.track.heliocentricPositionAtJulianDate(julianDate));
}

glm::dvec3 MissionEphemeris::voyagerRenderPosition(double julianDate) const
{
	const glm::dvec3 voyagerAu = m_voyager.heliocentricPositionAtJulianDate(julianDate);
	glm::dvec3 position = map(voyagerAu);

	auto clearPlanet = [&](const Planet& planet, double clearance)
	{
		const glm::dvec3 planetAu = planet.track.heliocentricPositionAtJulianDate(julianDate);
		glm::dvec3 fallback = Trajectory::eclipticToScene(voyagerAu - planetAu);
		fallback = glm::dot(fallback, fallback) > 0.0 ? glm::normalize(fallback) : glm::dvec3(0.0, 1.0, 0.0);
		position = applyClearance(position, map(planetAu), clearance, fallback);
	};

	if (const Planet* earth = findPlanet("earth"))
		clearPlanet(*earth, m_earthClearance);
	for (const Encounter& encounter : m_encounters)
	{
		if (const Planet* planet = findPlanet(encounter.planetId))
			clearPlanet(*planet, encounter.clearanceRenderUnits);
	}
	return position;
}

MissionEphemeris::Telemetry MissionEphemeris::voyagerTelemetry(double julianDate) const
{
	Telemetry telemetry;
	if (m_voyager.empty())
		return telemetry;

	const glm::dvec3 position = m_voyager.heliocentricPositionAtJulianDate(julianDate);
	telemetry.sunDistanceAu = glm::length(position);
	telemetry.heliocentricSpeedKmPerSecond = glm::length(m_voyager.velocityAtJulianDate(julianDate)) *
		ScaleManager::kKilometresPerAu / kSecondsPerDay;

	double nearest = 1e300;
	for (const Planet& planet : m_planets)
	{
		const double distance = glm::length(position - planet.track.heliocentricPositionAtJulianDate(julianDate));
		if (distance < nearest)
		{
			nearest = distance;
			telemetry.nearestPlanet = planet.displayName;
		}
	}
	telemetry.nearestPlanetDistanceKm = nearest * ScaleManager::kKilometresPerAu;
	return telemetry;
}

std::vector<glm::dvec3> MissionEphemeris::orbitGuide(const Planet& planet, double epochJulianDate,
	unsigned int segments) const
{
	std::vector<glm::dvec3> points;
	const glm::dvec3 r = planet.track.heliocentricPositionAtJulianDate(epochJulianDate);
	const glm::dvec3 v = planet.track.velocityAtJulianDate(epochJulianDate);
	const double radius = glm::length(r);
	if (radius <= 0.0 || segments < 3)
		return points;

	// Classical two-body elements from one state vector.
	const glm::dvec3 angularMomentum = glm::cross(r, v);
	const glm::dvec3 eccentricityVector = glm::cross(v, angularMomentum) / kSunGravitationalParameter - r / radius;
	const double eccentricity = glm::length(eccentricityVector);
	const double semiMajorAxis = 1.0 / (2.0 / radius - glm::dot(v, v) / kSunGravitationalParameter);
	if (semiMajorAxis <= 0.0 || eccentricity >= 1.0)
		return points;

	const glm::dvec3 perihelion = eccentricity > 1e-9 ? eccentricityVector / eccentricity : r / radius;
	const glm::dvec3 normal = glm::normalize(angularMomentum);
	const glm::dvec3 quadrature = glm::cross(normal, perihelion);
	const double semiMinorAxis = semiMajorAxis * std::sqrt(1.0 - eccentricity * eccentricity);

	points.reserve(segments);
	for (unsigned int i = 0; i < segments; ++i)
	{
		const double eccentricAnomaly = glm::two_pi<double>() * static_cast<double>(i) / segments;
		const glm::dvec3 point = perihelion * (semiMajorAxis * (std::cos(eccentricAnomaly) - eccentricity)) +
			quadrature * (semiMinorAxis * std::sin(eccentricAnomaly));
		points.push_back(map(point));
	}
	return points;
}
