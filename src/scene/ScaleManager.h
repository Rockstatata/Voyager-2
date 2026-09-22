#ifndef SCALE_MANAGER_H
#define SCALE_MANAGER_H

// The scene uses two explicit, documented scales because a literal model
// cannot show metre-scale Voyager and a 119-AU heliosphere together:
// 1) all celestial radii and moon-system distances share one linear factor,
//    preserving their exact ratios;
// 2) heliocentric distances use a monotonic power compression so the whole
//    mission remains navigable without packing the outer planets together.
class ScaleManager
{
public:
	double radiusToRenderUnits(double radiusKm) const;
	double sunRadiusToRenderUnits() const;
	double distanceToRenderUnits(double distanceKm) const;
	double moonOrbitDistanceToRenderUnits(double semiMajorAxisKm,
		double parentRadiusKm, double parentRenderRadius) const;
	double spacecraftSizeToRenderUnits(double sizeMeters) const;

private:
	static constexpr double kKilometresPerAu = 149597870.7;
	static constexpr double kSunRadiusKm = 696340.0;
	static constexpr double kSunRenderRadius = 1.5;
	static constexpr double kCelestialUnitsPerKm = kSunRenderRadius / kSunRadiusKm;
	static constexpr double kMercurySemiMajorAxisAu = 0.387098;
	static constexpr double kMercuryOrbitRenderRadius = 3.0;
	static constexpr double kHeliocentricCompressionExponent = 0.60;
	static constexpr double kSpacecraftUnitsPerMetre = 0.0001;
};

#endif
