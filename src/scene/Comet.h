#ifndef COMET_H
#define COMET_H

#include <memory>

#include "CelestialBody.h"

class Mesh;

// A mesh-less orbiting container with a nucleus, a glowing coma and a tail
// that is re-aimed every update so it always points away from the Sun, as a
// real comet's does (docs/objects/drifting-comet.md).
class Comet : public CelestialBody
{
public:
	Comet(const glm::dvec3& sunPosition, std::shared_ptr<Mesh> sphere);

	void update(double dt) override;

private:
	glm::dvec3 m_sunPosition;
	SceneObject* m_tail = nullptr; // non-owning; a child of this comet

	static constexpr float kTailLength = 9.0f;
};

#endif
