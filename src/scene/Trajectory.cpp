#include "Trajectory.h"

#include <algorithm>
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
	if (julianDate <= m_samples.front().julianDate)
		return m_samples.front().heliocentricAu + m_samples.front().velocityAuPerDay *
			(julianDate - m_samples.front().julianDate);
	if (julianDate >= m_samples.back().julianDate)
		return m_samples.back().heliocentricAu + m_samples.back().velocityAuPerDay *
			(julianDate - m_samples.back().julianDate);

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
	if (julianDate <= m_samples.front().julianDate)
		return m_samples.front().velocityAuPerDay;
	if (julianDate >= m_samples.back().julianDate)
		return m_samples.back().velocityAuPerDay;

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
