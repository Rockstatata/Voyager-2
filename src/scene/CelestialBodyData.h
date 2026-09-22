#ifndef CELESTIAL_BODY_DATA_H
#define CELESTIAL_BODY_DATA_H

#include <string>

#include "BodyType.h"

// Bible section 18. Physical/orbital facts about one body, independent of
// how it's currently drawn (render position/radius are a separate mapping —
// ScaleManager, Phase 4 — so this struct never changes when that mapping does).
struct CelestialBodyData
{
	std::string id;           // registry key, e.g. "earth"
	std::string displayName;  // UI/label text, e.g. "Earth"
	std::string parentId;     // empty for bodies with no orbital parent (the Sun)

	double radiusKm = 0.0;
	double semiMajorAxisKm = 0.0;      // 0 for the Sun (no orbit)
	double eccentricity = 0.0;         // 0 = circle; real orbits are 0.0002 (Tethys) to 0.25 (Pluto)
	double orbitalPeriodDays = 0.0;    // 0 for the Sun
	double rotationPeriodHours = 0.0;  // negative marks retrograde spin (Venus, Uranus)
	double axialTiltDegrees = 0.0;

	std::string texturePath;  // relative to project root, per Texture2D::loadFromFile
	std::string materialId;   // free-form tag for docs/debugging, not yet used by rendering

	BodyType type = BodyType::Planet;
};

#endif
