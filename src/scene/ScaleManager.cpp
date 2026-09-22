#include "ScaleManager.h"

#include <algorithm>
#include <cmath>

double ScaleManager::radiusToRenderUnits(double radiusKm) const
{
	return radiusKm * kCelestialUnitsPerKm;
}

double ScaleManager::sunRadiusToRenderUnits() const
{
	return radiusToRenderUnits(kSunRadiusKm);
}

double ScaleManager::distanceToRenderUnits(double distanceKm) const
{
	if (distanceKm <= 0.0)
		return 0.0;

	const double distanceAu = distanceKm / kKilometresPerAu;
	return kMercuryOrbitRenderRadius * std::pow(
		distanceAu / kMercurySemiMajorAxisAu, kHeliocentricCompressionExponent);
}

double ScaleManager::moonOrbitDistanceToRenderUnits(double semiMajorAxisKm,
	double parentRadiusKm, double parentRenderRadius) const
{
	if (semiMajorAxisKm <= 0.0 || parentRadiusKm <= 0.0 || parentRenderRadius <= 0.0)
		return 0.0;

	// Moon systems are too large to read beside their planets at the overview
	// scale when mapped linearly. Compress only the parent-relative orbit
	// distance: order and ring clearance stay intact, while the body radii
	// themselves retain their exact shared linear ratio.
	const double parentRelativeDistance = semiMajorAxisKm / parentRadiusKm;
	const double compressedParentRelativeDistance = std::pow(parentRelativeDistance, 0.70);
	return parentRenderRadius * std::max(compressedParentRelativeDistance, 3.0);
}

double ScaleManager::spacecraftSizeToRenderUnits(double sizeMeters) const
{
	return sizeMeters * kSpacecraftUnitsPerMetre;
}
