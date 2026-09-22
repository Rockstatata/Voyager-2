#ifndef BODY_TYPE_H
#define BODY_TYPE_H

// Bible section 18 — classifies a CelestialBodyData entry for registry
// queries (e.g. "list all moons") and future per-type rendering rules
// (rings only apply to Planet, oblateness only to gas giants, etc).
enum class BodyType
{
	Star,
	Planet,
	DwarfPlanet,
	Moon
};

#endif
