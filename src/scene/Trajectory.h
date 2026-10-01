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

	// Outside the table's dates a track is extrapolated. By default it
	// coasts in a straight line at the end velocity (right for Voyager, on a
	// nearly straight escape path). With orbital extrapolation on (planets)
	// it follows the two-body Kepler orbit through the end state vector, so
	// planets keep circling the Sun after the NASA/JPL data ends.
	void setOrbitalExtrapolation(bool enabled) { m_orbitalExtrapolation = enabled; }

	// Two-body motion about the Sun: the state (r0, v0) advanced by `days`
	// along its ellipse (Kepler's equation). False for unbound orbits.
	static bool propagateKepler(const glm::dvec3& r0, const glm::dvec3& v0, double days,
		glm::dvec3& position, glm::dvec3& velocity);

	// The Sun's GM in AU^3/day^2 (Gaussian gravitational constant squared).
	static constexpr double kSunGravitationalParameter = 2.9591220828559115e-4;

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
	// The extrapolated state outside the table, or false inside it.
	bool extrapolate(double julianDate, glm::dvec3& position, glm::dvec3& velocity) const;

	bool m_hasVelocities = false;
	bool m_orbitalExtrapolation = false;
	std::vector<TrajectorySample> m_samples;
};

#endif
