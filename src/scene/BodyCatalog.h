#ifndef BODY_CATALOG_H
#define BODY_CATALOG_H

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "CelestialBodyData.h"

// Data-driven scene content (bible R6 and section 44). Physical facts, ring
// bands and mission bookmarks live in CSV files under assets/data/, so adding
// a moon or retuning a ring never needs a code change. Pure file parsing: no
// OpenGL, no scene knowledge.
struct RingBandData
{
	std::string planetId;
	std::string name;
	float innerRadius = 0.0f; // in parent-planet radii
	float outerRadius = 0.0f;
	glm::vec3 color{ 1.0f };
	float opacity = 1.0f;
};

struct BookmarkData
{
	int key = 0;              // 1..9
	std::string name;
	std::string planetId;     // empty for a dated bookmark
	double julianDate = 0.0;  // 0 = start of the Voyager table
	double leadDays = 0.0;    // days before closest approach
};

class BodyCatalog
{
public:
	static std::vector<CelestialBodyData> loadBodies(const std::string& path);
	static std::vector<RingBandData> loadRingBands(const std::string& path);
	static std::vector<BookmarkData> loadBookmarks(const std::string& path);
};

#endif
