#include "Comet.h"

#include <cmath>

#include <glm/gtc/constants.hpp>

#include "ScaleManager.h"
#include "../rendering/CylinderGenerator.h"
#include "../rendering/MaterialLibrary.h"
#include "../rendering/Mesh.h"

namespace
{
	CelestialBodyData cometData()
	{
		CelestialBodyData data;
		data.id = "drifting_comet";
		data.displayName = "Drifting Comet";
		return data;
	}
}

Comet::Comet(const glm::dvec3& sunPosition, std::shared_ptr<Mesh> sphere)
	: CelestialBody(cometData(), nullptr, nullptr), m_sunPosition(sunPosition)
{
	transform().scale = glm::dvec3(1.0);
	// 60 AU, eccentricity 0.35, one revolution in ~13 minutes at 1x.
	setOrbit(ScaleManager().distanceAuToRenderUnits(60.0), 0.008, 4.712389, sunPosition, 0.35);

	auto nucleus = std::make_unique<SceneObject>("drifting_comet_nucleus");
	nucleus->setMesh(sphere);
	nucleus->setMaterial(MaterialLibrary::flat(glm::vec3(0.82f, 0.92f, 1.0f)));
	nucleus->transform().scale = glm::dvec3(0.25);
	addChild(std::move(nucleus));

	auto coma = std::make_unique<SceneObject>("drifting_comet_coma");
	coma->setMesh(sphere);
	coma->setMaterial(MaterialLibrary::glow(glm::vec3(0.55f, 0.85f, 1.0f), 0.8f, 2.0f));
	coma->transform().scale = glm::dvec3(0.9);
	addChild(std::move(coma));

	auto tail = std::make_unique<SceneObject>("drifting_comet_tail");
	tail->setMesh(std::make_shared<Mesh>(CylinderGenerator::generate(0.35f, 0.0f, kTailLength, 16)));
	tail->setMaterial(MaterialLibrary::flat(glm::vec3(0.62f, 0.88f, 1.0f), ShadingModel::Unlit, 0.45f));
	m_tail = tail.get();
	addChild(std::move(tail));
}

void Comet::update(double dt)
{
	CelestialBody::update(dt);

	// Rotate the cone's +Y axis onto the anti-Sun direction, then slide it by
	// half its length so the wide end stays on the nucleus.
	const glm::dvec3 awayFromSun = glm::normalize(transform().position - m_sunPosition);
	const glm::dvec3 yAxis(0.0, 1.0, 0.0);
	const glm::dvec3 axis = glm::cross(yAxis, awayFromSun);
	const double axisLength = glm::length(axis);
	glm::dquat rotation(1.0, 0.0, 0.0, 0.0);
	if (axisLength < 1e-9)
	{
		if (glm::dot(yAxis, awayFromSun) < 0.0)
			rotation = glm::angleAxis(glm::pi<double>(), glm::dvec3(1.0, 0.0, 0.0));
	}
	else
	{
		rotation = glm::angleAxis(std::acos(glm::clamp(glm::dot(yAxis, awayFromSun), -1.0, 1.0)), axis / axisLength);
	}

	m_tail->transform().rotation = rotation;
	m_tail->transform().position = rotation * glm::dvec3(0.0, kTailLength * 0.5, 0.0);
}
