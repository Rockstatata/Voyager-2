#include "Trajectory.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

#include "ScaleManager.h"

bool Trajectory::loadCsv(const std::string& path)
{
	m_samples.clear();
	m_hasVelocities = true;
	std::ifstream file(path);
	if (!file)
	{
		std::cout << "[TRAJECTORY] failed to open " << path << std::endl;
		return false;
	}

	std::string line;
	while (std::getline(file, line))
	{
		if (line.empty() || line.front() == '#' || line.rfind("julian_date", 0) == 0)
			continue;

		std::stringstream row(line);
		std::string field;
		std::vector<double> values;
		try
		{
			while (std::getline(row, field, ','))
			{
				if (!field.empty())
					values.push_back(std::stod(field));
			}
		}
		catch (const std::exception&)
		{
			std::cout << "[TRAJECTORY] malformed row skipped: " << line << std::endl;
			continue;
		}
		if (values.size() < 4)
			continue;

		TrajectorySample sample;
		sample.julianDate = values[0];
		sample.heliocentricAu = glm::dvec3(values[1], values[2], values[3]);
		if (values.size() >= 7)
			sample.velocityAuPerDay = glm::dvec3(values[4], values[5], values[6]);
		else
			m_hasVelocities = false;
		m_samples.push_back(sample);
	}

	std::sort(m_samples.begin(), m_samples.end(),
		[](const TrajectorySample& a, const TrajectorySample& b)
		{
			return a.julianDate < b.julianDate;
		});

	std::cout << "[TRAJECTORY] loaded " << m_samples.size() << " NASA/JPL Horizons "
		<< (m_hasVelocities ? "state vectors" : "positions") << " from " << path << std::endl;
	return m_samples.size() >= 2;
}

glm::dvec3 Trajectory::heliocentricPositionAtJulianDate(double julianDate) const
{
	if (m_samples.empty())
		return glm::dvec3(0.0);
	glm::dvec3 position;
	glm::dvec3 velocity;
	if (extrapolate(julianDate, position, velocity))
		return position;

	const auto upper = std::upper_bound(m_samples.begin(), m_samples.end(), julianDate,
		[](double value, const TrajectorySample& sample)
		{
			return value < sample.julianDate;
		});
	const TrajectorySample& after = *upper;
	const TrajectorySample& before = *(upper - 1);
	const double span = after.julianDate - before.julianDate;
	const double t = (julianDate - before.julianDate) / span;
	if (!m_hasVelocities)
		return glm::mix(before.heliocentricAu, after.heliocentricAu, t);

	// Cubic Hermite basis: position and derivative match at both rows.
	const double t2 = t * t;
	const double t3 = t2 * t;
	const double h00 = 2.0 * t3 - 3.0 * t2 + 1.0;
	const double h10 = t3 - 2.0 * t2 + t;
	const double h01 = -2.0 * t3 + 3.0 * t2;
	const double h11 = t3 - t2;
	return h00 * before.heliocentricAu + h10 * span * before.velocityAuPerDay +
		h01 * after.heliocentricAu + h11 * span * after.velocityAuPerDay;
}

glm::dvec3 Trajectory::velocityAtJulianDate(double julianDate) const
{
	if (m_samples.size() < 2)
		return glm::dvec3(0.0);
	if (!m_hasVelocities)
	{
		constexpr double kStepDays = 0.01;
		return (heliocentricPositionAtJulianDate(julianDate + kStepDays) -
			heliocentricPositionAtJulianDate(julianDate - kStepDays)) / (2.0 * kStepDays);
	}
	glm::dvec3 position;
	glm::dvec3 velocity;
	if (extrapolate(julianDate, position, velocity))
		return velocity;

	const auto upper = std::upper_bound(m_samples.begin(), m_samples.end(), julianDate,
		[](double value, const TrajectorySample& sample)
		{
			return value < sample.julianDate;
		});
	const TrajectorySample& after = *upper;
	const TrajectorySample& before = *(upper - 1);
	const double span = after.julianDate - before.julianDate;
	const double t = (julianDate - before.julianDate) / span;
	const double t2 = t * t;
	// Derivative of the Hermite basis, divided by span to return AU/day.
	const double d00 = 6.0 * t2 - 6.0 * t;
	const double d10 = 3.0 * t2 - 4.0 * t + 1.0;
	const double d01 = -6.0 * t2 + 6.0 * t;
	const double d11 = 3.0 * t2 - 2.0 * t;
	return (d00 * before.heliocentricAu + d01 * after.heliocentricAu) / span +
		d10 * before.velocityAuPerDay + d11 * after.velocityAuPerDay;
}

glm::dvec3 Trajectory::mapHeliocentricToRender(const glm::dvec3& sceneAu,
	const ScaleManager& scaleManager, const glm::dvec3& sunPosition)
{
	const double distanceAu = glm::length(sceneAu);
	if (distanceAu <= 1e-12)
		return sunPosition;
	return sunPosition + (sceneAu / distanceAu) * scaleManager.distanceAuToRenderUnits(distanceAu);
}

bool Trajectory::extrapolate(double julianDate, glm::dvec3& position, glm::dvec3& velocity) const
{
	const bool before = julianDate < m_samples.front().julianDate;
	const bool after = julianDate > m_samples.back().julianDate;
	if (!before && !after)
		return false;
	const TrajectorySample& edge = before ? m_samples.front() : m_samples.back();
	const double days = julianDate - edge.julianDate;
	if (m_orbitalExtrapolation && propagateKepler(edge.heliocentricAu, edge.velocityAuPerDay, days, position, velocity))
		return true;
	position = edge.heliocentricAu + edge.velocityAuPerDay * days;
	velocity = edge.velocityAuPerDay;
	return true;
}

bool Trajectory::propagateKepler(const glm::dvec3& r0, const glm::dvec3& v0, double days,
	glm::dvec3& position, glm::dvec3& velocity)
{
	constexpr double mu = kSunGravitationalParameter;
	const double radius = glm::length(r0);
	if (radius <= 0.0)
		return false;

	// Orbital elements from one state vector: the angular momentum fixes the
	// plane, the eccentricity vector points at perihelion, and the energy
	// gives the semi-major axis (vis-viva).
	const glm::dvec3 angularMomentum = glm::cross(r0, v0);
	const double semiMajorAxis = 1.0 / (2.0 / radius - glm::dot(v0, v0) / mu);
	const glm::dvec3 eccentricityVector = glm::cross(v0, angularMomentum) / mu - r0 / radius;
	const double eccentricity = glm::length(eccentricityVector);
	if (semiMajorAxis <= 0.0 || eccentricity >= 1.0 || glm::length(angularMomentum) <= 0.0)
		return false;

	const glm::dvec3 perihelion = eccentricity > 1e-9 ? eccentricityVector / eccentricity : r0 / radius;
	const glm::dvec3 quadrature = glm::cross(glm::normalize(angularMomentum), perihelion);
	const double semiMinorAxis = semiMajorAxis * std::sqrt(1.0 - eccentricity * eccentricity);

	// Where on the ellipse r0 is: x = a (cos E - e), y = b sin E.
	const double startAnomaly = std::atan2(glm::dot(r0, quadrature) / semiMinorAxis,
		glm::dot(r0, perihelion) / semiMajorAxis + eccentricity);
	const double meanMotion = std::sqrt(mu / (semiMajorAxis * semiMajorAxis * semiMajorAxis));
	const double meanAnomaly = startAnomaly - eccentricity * std::sin(startAnomaly) + meanMotion * days;

	// Kepler's equation M = E - e sin E by Newton's method.
	double anomaly = meanAnomaly;
	for (int iteration = 0; iteration < 12; ++iteration)
	{
		const double step = (anomaly - eccentricity * std::sin(anomaly) - meanAnomaly) /
			(1.0 - eccentricity * std::cos(anomaly));
		anomaly -= step;
		if (std::abs(step) < 1e-12)
			break;
	}

	const double cosE = std::cos(anomaly);
	const double sinE = std::sin(anomaly);
	const double anomalyRate = meanMotion / (1.0 - eccentricity * cosE);
	position = perihelion * (semiMajorAxis * (cosE - eccentricity)) + quadrature * (semiMinorAxis * sinE);
	velocity = perihelion * (-semiMajorAxis * sinE * anomalyRate) + quadrature * (semiMinorAxis * cosE * anomalyRate);
	return true;
}
