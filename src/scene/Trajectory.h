#ifndef TRAJECTORY_H
#define TRAJECTORY_H

#include <string>
#include <vector>

#include <glm/glm.hpp>

class ScaleManager;

struct TrajectorySample
{
	double julianDate = 0.0;
	glm::dvec3 heliocentricAu{ 0.0 };
};

// Offline NASA/JPL Horizons samples. Loading and physical-to-render mapping
// stay separate so source data remain auditable and never depend on the
// educational display scale stored in today's renderer.
class Trajectory
{
public:
	bool loadCsv(const std::string& path);

	const std::vector<TrajectorySample>& samples() const { return m_samples; }
	glm::dvec3 heliocentricPositionAtJulianDate(double julianDate) const;
	glm::dvec3 renderPositionAtJulianDate(double julianDate,
		const ScaleManager& scaleManager, const glm::dvec3& sunPosition) const;
	std::vector<glm::dvec3> buildRenderPath(
		const ScaleManager& scaleManager, const glm::dvec3& sunPosition) const;

private:
	std::vector<TrajectorySample> m_samples;
};

#endif
