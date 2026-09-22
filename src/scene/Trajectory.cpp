#include "Trajectory.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

#include "ScaleManager.h"

namespace
{
	constexpr double kKilometresPerAu = 149597870.7;
}

bool Trajectory::loadCsv(const std::string& path)
{
	m_samples.clear();
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
		TrajectorySample sample;
		try
		{
			if (!std::getline(row, field, ',')) continue;
			sample.julianDate = std::stod(field);
			if (!std::getline(row, field, ',')) continue;
			sample.heliocentricAu.x = std::stod(field);
			if (!std::getline(row, field, ',')) continue;
			sample.heliocentricAu.y = std::stod(field);
			if (!std::getline(row, field, ',')) continue;
			sample.heliocentricAu.z = std::stod(field);
		}
		catch (const std::exception&)
		{
			std::cout << "[TRAJECTORY] malformed row skipped: " << line << std::endl;
			continue;
		}
		m_samples.push_back(sample);
	}

	std::cout << "[TRAJECTORY] loaded " << m_samples.size()
		<< " NASA/JPL Horizons samples from " << path << std::endl;
	return m_samples.size() >= 2;
}

glm::dvec3 Trajectory::heliocentricPositionAtJulianDate(double julianDate) const
{
	if (m_samples.empty())
		return glm::dvec3(0.0);
	if (julianDate <= m_samples.front().julianDate)
		return m_samples.front().heliocentricAu;
	if (julianDate >= m_samples.back().julianDate)
		return m_samples.back().heliocentricAu;

	const auto upper = std::upper_bound(m_samples.begin(), m_samples.end(), julianDate,
		[](double value, const TrajectorySample& sample)
		{
			return value < sample.julianDate;
		});
	const TrajectorySample& after = *upper;
	const TrajectorySample& before = *(upper - 1);
	const double fraction = (julianDate - before.julianDate) /
		(after.julianDate - before.julianDate);
	return glm::mix(before.heliocentricAu, after.heliocentricAu, fraction);
}

glm::dvec3 Trajectory::renderPositionAtJulianDate(double julianDate,
	const ScaleManager& scaleManager, const glm::dvec3& sunPosition) const
{
	const glm::dvec3 heliocentricAu = heliocentricPositionAtJulianDate(julianDate);
	const glm::dvec3 sceneVector(heliocentricAu.x, heliocentricAu.z, heliocentricAu.y);
	const double distanceAu = glm::length(sceneVector);
	if (distanceAu <= 1e-12)
		return sunPosition;

	const double renderDistance = scaleManager.distanceToRenderUnits(distanceAu * kKilometresPerAu);
	return sunPosition + glm::normalize(sceneVector) * renderDistance;
}

std::vector<glm::dvec3> Trajectory::buildRenderPath(
	const ScaleManager& scaleManager, const glm::dvec3& sunPosition) const
{
	std::vector<glm::dvec3> path;
	path.reserve(m_samples.size());
	for (const TrajectorySample& sample : m_samples)
	{
		path.push_back(renderPositionAtJulianDate(sample.julianDate, scaleManager, sunPosition));
	}
	return path;
}
