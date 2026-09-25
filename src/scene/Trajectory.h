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
	glm::dvec3 velocityAuPerDay{ 0.0 };
};

// Offline NASA/JPL Horizons state vectors (ECLIPTIC/ICRF, AU and AU/day).
// Loading and physical-to-render mapping stay separate so source data remain
// auditable and never depend on the educational display scale.
//
// When a table carries velocities, positions are cubic-Hermite interpolated:
// the curve matches both the sampled position AND the sampled velocity at
// every row, so a planet sampled every few days (or Voyager every 10 minutes
// at Neptune) follows a smooth, physically shaped arc instead of a polyline.
class Trajectory
{
public:
	bool loadCsv(const std::string& path);

	const std::vector<TrajectorySample>& samples() const { return m_samples; }
	bool empty() const { return m_samples.empty(); }
	double startJulianDate() const { return m_samples.empty() ? 0.0 : m_samples.front().julianDate; }
	double endJulianDate() const { return m_samples.empty() ? 0.0 : m_samples.back().julianDate; }

	glm::dvec3 heliocentricPositionAtJulianDate(double julianDate) const;
	glm::dvec3 velocityAtJulianDate(double julianDate) const;

	// Heliocentric AU (ecliptic X, Y, Z) to scene axes (X, Z, Y): the ecliptic
	// is the scene's horizontal plane and ecliptic north is scene +Y.
	static glm::dvec3 eclipticToScene(const glm::dvec3& ecliptic)
	{
		return glm::dvec3(ecliptic.x, ecliptic.z, ecliptic.y);
	}

	// Direction preserved, heliocentric distance compressed by ScaleManager.
	static glm::dvec3 mapHeliocentricToRender(const glm::dvec3& sceneAu,
		const ScaleManager& scaleManager, const glm::dvec3& sunPosition);

private:
	bool m_hasVelocities = false;
	std::vector<TrajectorySample> m_samples;
};

#endif
