#include "BodyCatalog.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace
{
	// Rows of comma-separated fields; '#' comments and the header row are
	// skipped. Every row must have exactly `columns` fields.
	std::vector<std::vector<std::string>> readTable(const std::string& path, std::size_t columns)
	{
		std::vector<std::vector<std::string>> rows;
		std::ifstream file(path);
		if (!file)
		{
			std::cout << "[ASSET] failed to open " << path << std::endl;
			return rows;
		}

		std::string line;
		bool headerSkipped = false;
		while (std::getline(file, line))
		{
			if (!line.empty() && line.back() == '\r')
				line.pop_back();
			if (line.empty() || line.front() == '#')
				continue;
			if (!headerSkipped)
			{
				headerSkipped = true;
				continue;
			}

			std::vector<std::string> fields;
			std::stringstream row(line);
			std::string field;
			while (std::getline(row, field, ','))
				fields.push_back(field);
			if (line.back() == ',')
				fields.emplace_back();
			if (fields.size() != columns)
			{
				std::cout << "[ASSET] " << path << ": skipped row with " << fields.size()
						  << " fields (expected " << columns << "): " << line << std::endl;
				continue;
			}
			rows.push_back(std::move(fields));
		}
		return rows;
	}

	double number(const std::string& text)
	{
		return text.empty() ? 0.0 : std::stod(text);
	}

	BodyType bodyType(const std::string& text)
	{
		if (text == "star") return BodyType::Star;
		if (text == "dwarf") return BodyType::DwarfPlanet;
		if (text == "moon") return BodyType::Moon;
		return BodyType::Planet;
	}
}

std::vector<CelestialBodyData> BodyCatalog::loadBodies(const std::string& path)
{
	std::vector<CelestialBodyData> bodies;
	for (const auto& fields : readTable(path, 12))
	{
		CelestialBodyData data;
		data.id = fields[0];
		data.displayName = fields[1];
		data.parentId = fields[2];
		data.radiusKm = number(fields[3]);
		data.semiMajorAxisKm = number(fields[4]);
		data.eccentricity = number(fields[5]);
		data.orbitalPeriodDays = number(fields[6]);
		data.rotationPeriodHours = number(fields[7]);
		data.axialTiltDegrees = number(fields[8]);
		data.texturePath = fields[9];
		data.materialId = fields[10];
		data.type = bodyType(fields[11]);
		bodies.push_back(std::move(data));
	}
	std::cout << "[SOLAR] catalog: " << bodies.size() << " bodies from " << path << std::endl;
	return bodies;
}

std::vector<RingBandData> BodyCatalog::loadRingBands(const std::string& path)
{
	std::vector<RingBandData> bands;
	for (const auto& fields : readTable(path, 8))
	{
		RingBandData band;
		band.planetId = fields[0];
		band.name = fields[1];
		band.innerRadius = static_cast<float>(number(fields[2]));
		band.outerRadius = static_cast<float>(number(fields[3]));
		band.color = glm::vec3(static_cast<float>(number(fields[4])), static_cast<float>(number(fields[5])),
			static_cast<float>(number(fields[6])));
		band.opacity = static_cast<float>(number(fields[7]));
		bands.push_back(std::move(band));
	}
	return bands;
}

std::vector<BookmarkData> BodyCatalog::loadBookmarks(const std::string& path)
{
	std::vector<BookmarkData> bookmarks;
	for (const auto& fields : readTable(path, 5))
	{
		BookmarkData bookmark;
		bookmark.key = static_cast<int>(number(fields[0]));
		bookmark.name = fields[1];
		bookmark.planetId = fields[2];
		bookmark.julianDate = number(fields[3]);
		bookmark.leadDays = number(fields[4]);
		bookmarks.push_back(std::move(bookmark));
	}
	return bookmarks;
}
