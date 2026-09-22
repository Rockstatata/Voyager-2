#ifndef CELESTIAL_BODY_H
#define CELESTIAL_BODY_H

#include <memory>

#include "CelestialBodyData.h"
#include "SceneObject.h"

// Bible section 22/52. A SceneObject that knows its own physical facts and
// animates its own axial spin from them, instead of Application hand-writing
// a quaternion per body (the Phase-2 approach this replaces).
class CelestialBody : public SceneObject
{
public:
	CelestialBody(CelestialBodyData data, std::shared_ptr<Mesh> sphere,
				  std::shared_ptr<Material> material);

	void update(double dt) override;

	const CelestialBodyData& data() const { return m_data; }

	// Phase 5 elliptical-orbit placeholder (bible: "circular orbit
	// approximation" was the prior step; this is the next one — still not
	// real Keplerian motion, see updateOrbitPosition). `semiMajorAxis`/
	// `angularVelocity` are already-converted render units / radians-per-
	// second — CelestialBody just integrates an angle and writes position,
	// it doesn't know or care whether this is a planet orbiting the Sun or
	// a moon orbiting a planet. `center` is in THIS body's own local space:
	// (0,0,0) for a moon (whose parent IS the planet, so local space is
	// already planet-relative — the scene graph does that translation for
	// free), or the Sun's absolute world position for a planet (a scene
	// root, with no parent to supply that translation automatically).
	// Calling this also sets the initial position immediately, so code that
	// reads position() right after construction (e.g. the orbit-preview
	// rings) sees a valid point.
	void setOrbit(double semiMajorAxis, double angularVelocity, double initialAngleRadians,
				   const glm::dvec3& center = glm::dvec3(0.0), double eccentricity = 0.0);

	// Simulation-wide pause/speed multiplier applied to orbit AND spin
	// motion (bible section 21, "pause and speed controls affect the full
	// system consistently") — not to Input/Camera, which stay on real dt so
	// piloting never feels laggy regardless of simulation speed.
	static void setSimulationTimeScale(double scale) { s_simulationTimeScale = scale; }

private:
	void updateOrbitPosition();

	CelestialBodyData m_data;

	bool m_hasOrbit = false;
	double m_orbitSemiMajorAxis = 0.0;
	double m_orbitEccentricity = 0.0;
	double m_orbitAngularVelocity = 0.0;
	double m_orbitAngleRadians = 0.0;
	glm::dvec3 m_orbitCenter{ 0.0 };

	static inline double s_simulationTimeScale = 1.0;

	// Demo-visible spin: real rotation periods range from ~10 hours (Jupiter)
	// to ~5833 hours (Venus), so real-time spin would look motionless for
	// most bodies. kVisualSpinSpeedup compresses time by a constant factor —
	// every body's spin rate stays proportional to its real one, only the
	// clock is sped up, so the education point (Jupiter spins faster than
	// Venus, Venus/Uranus spin backwards) still holds. ScaleManager/
	// SimulationClock (Phase 4-5) replace this with the real simulation
	// clock; this field is deliberately not `static` so that swap is a
	// single assignment, not a code shape change.
	static constexpr double kVisualSpinSpeedup = 3600.0; // 1 sim-hour per real second
	double m_spinAngleRadians = 0.0;
};

#endif
