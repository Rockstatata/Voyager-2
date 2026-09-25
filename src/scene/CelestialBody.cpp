#include "CelestialBody.h"

#include <cmath>

#include <glm/gtc/constants.hpp>

#include "../rendering/Renderer.h"

CelestialBody::CelestialBody(CelestialBodyData data, std::shared_ptr<Mesh> sphere,
							  std::shared_ptr<Material> material)
	: SceneObject(data.id), m_data(std::move(data))
{
	setMesh(std::move(sphere));
	setMaterial(std::move(material));

	const double tiltRadians = glm::radians(m_data.axialTiltDegrees);
	transform().rotation = glm::angleAxis(tiltRadians, glm::dvec3(0.0, 0.0, 1.0));
}

void CelestialBody::setOrbit(double semiMajorAxis, double angularVelocity, double initialAngleRadians,
							  const glm::dvec3& center, double eccentricity)
{
	m_hasOrbit = true;
	m_orbitSemiMajorAxis = semiMajorAxis;
	m_orbitEccentricity = eccentricity;
	m_orbitAngularVelocity = angularVelocity;
	m_orbitAngleRadians = initialAngleRadians;
	m_orbitCenter = center;
	updateOrbitPosition();
}

void CelestialBody::updateOrbitPosition()
{
	// m_orbitAngleRadians is treated as true anomaly (angle measured from
	// the focus, i.e. from the Sun/planet this body orbits) in the standard
	// polar conic-section equation r = a(1-e^2)/(1+e*cos(theta)) — this
	// traces an exact ellipse with the orbited body at the focus, exactly
	// like a real orbit's geometry. The simplification is the RATE: a real
	// orbit sweeps equal areas in equal times (faster at perihelion, per
	// Kepler's second law), which requires numerically solving Kepler's
	// equation; this advances theta at a constant rate instead (see
	// update()), so the shape is correct but the speed isn't real-physical
	// moment-to-moment. e=0 reduces r to a constant (a circle), so this is
	// a strict superset of the Phase-3/early-Phase-5 circular version.
	const double eccentricitySquared = m_orbitEccentricity * m_orbitEccentricity;
	const double r = m_orbitSemiMajorAxis * (1.0 - eccentricitySquared) /
		(1.0 + m_orbitEccentricity * std::cos(m_orbitAngleRadians));
	transform().position = m_orbitCenter + r *
		glm::dvec3(std::cos(m_orbitAngleRadians), 0.0, std::sin(m_orbitAngleRadians));
}

void CelestialBody::update(double dt)
{
	const double simDt = dt * s_simulationTimeScale;

	if (m_hasOrbit)
	{
		m_orbitAngleRadians += m_orbitAngularVelocity * simDt;
		updateOrbitPosition();
	}

	if (m_data.rotationPeriodHours != 0.0)
	{
		const double rotationPeriodSeconds = m_data.rotationPeriodHours * 3600.0;
		const double angularVelocity = (2.0 * glm::pi<double>() / rotationPeriodSeconds) * kVisualSpinSpeedup;
		m_spinAngleRadians += angularVelocity * simDt;
	}

	// The transform carries only the axial tilt (the equatorial frame that
	// children inherit); the spin is applied to this body's own mesh in render().
	const double tiltRadians = glm::radians(m_data.axialTiltDegrees);
	transform().rotation = glm::angleAxis(tiltRadians, glm::dvec3(0.0, 0.0, 1.0));

	SceneObject::update(dt);
}

glm::dmat4 CelestialBody::surfaceMatrix() const
{
	return worldMatrix() * glm::mat4_cast(glm::angleAxis(m_spinAngleRadians, glm::dvec3(0.0, 1.0, 0.0)));
}

void CelestialBody::render(Renderer& renderer)
{
	if (!visible())
		return;

	if (mesh() != nullptr && material() != nullptr)
	{
		renderer.submit(*mesh(), *material(), surfaceMatrix());
	}

	for (const auto& child : children())
		child->render(renderer);
}
