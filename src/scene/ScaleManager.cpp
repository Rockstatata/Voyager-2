#include "ScaleManager.h"

#include <cmath>

double ScaleManager::radiusToRenderUnits(double radiusKm) const
{
	if (radiusKm <= 0.0)
		return 0.0;

	// Power-law compression around Earth: Earth is 0.50 units, Jupiter 2.10,
	// Mercury 0.28 and Miranda 0.07. Every size comparison keeps its direction
	// (Ganymede > Titan > Mercury), but a moon no longer vanishes beside its
	// planet the way it does at a strictly linear scale.
	return kEarthRenderRadius * std::pow(radiusKm / kEarthRadiusKm, kRadiusCompressionExponent);
}

double ScaleManager::sunRadiusToRenderUnits() const
{
	return kSunRenderRadius;
}

double ScaleManager::distanceToRenderUnits(double distanceKm) const
{
	return distanceAuToRenderUnits(distanceKm / kKilometresPerAu);
}

double ScaleManager::distanceAuToRenderUnits(double distanceAu) const
{
	if (distanceAu <= 0.0)
		return 0.0;

	return kMercuryOrbitRenderRadius * std::pow(
		distanceAu / kMercurySemiMajorAxisAu, kHeliocentricCompressionExponent);
}

double ScaleManager::distanceScaleAtAu(double distanceAu) const
{
	if (distanceAu <= 1e-9)
		return 0.0;
	return kHeliocentricCompressionExponent * distanceAuToRenderUnits(distanceAu) / distanceAu;
}

double ScaleManager::moonOrbitDistanceToRenderUnits(double semiMajorAxisKm,
	double parentRadiusKm, double parentRenderRadius) const
{
	if (semiMajorAxisKm <= 0.0 || parentRadiusKm <= 0.0 || parentRenderRadius <= 0.0)
		return 0.0;

	// Parent-relative distance x = a / R. Rendered as R' (1.6 + sqrt(x)):
	// monotonic, so real orbital order is kept, and the innermost moons
	// (Tethys x=5.1, Miranda x=5.1) land at ~3.9 parent radii — outside
	// Saturn's F ring (2.42) and Uranus's epsilon ring (2.03).
	const double parentRelativeDistance = semiMajorAxisKm / parentRadiusKm;
	return parentRenderRadius * (kMoonOrbitBaseInParentRadii +
		std::pow(parentRelativeDistance, kMoonOrbitCompressionExponent));
}

double ScaleManager::spacecraftSizeToRenderUnits(double sizeMeters) const
{
	return sizeMeters * kSpacecraftUnitsPerMetre;
}
