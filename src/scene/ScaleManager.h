#ifndef SCALE_MANAGER_H
#define SCALE_MANAGER_H

// Educational presentation scale (bible section 19). A literal model cannot
// show metre-scale Voyager, 25,000 km planets and a 119-AU heliosphere in one
// navigable scene, so each quantity has its own documented monotonic mapping:
// 1) body radii use one power law relative to Earth, so every planet and moon
//    stays in the correct size ORDER while small worlds remain visible (the
//    Sun alone is capped so it does not swallow Mercury's orbit);
// 2) heliocentric distances use one power law relative to Mercury's orbit;
// 3) moon orbits use a parent-relative square-root compression that keeps
//    their real order and clears every ring system;
// 4) spacecraft metres use one linear factor for every Voyager component.
// Physical values in CelestialBodyData and the ephemeris tables never change.
class ScaleManager
{
public:
	double radiusToRenderUnits(double radiusKm) const;
	double sunRadiusToRenderUnits() const;
	double distanceToRenderUnits(double distanceKm) const;
	double distanceAuToRenderUnits(double distanceAu) const;
	// d(render distance)/d(AU) at a heliocentric distance: the local
	// magnification of the heliocentric mapping.
	double distanceScaleAtAu(double distanceAu) const;
	double moonOrbitDistanceToRenderUnits(double semiMajorAxisKm,
		double parentRadiusKm, double parentRenderRadius) const;
	double spacecraftSizeToRenderUnits(double sizeMeters) const;

	static constexpr double kKilometresPerAu = 149597870.7;
	static constexpr double kEarthRadiusKm = 6371.0;
	static constexpr double kEarthRenderRadius = 0.50;
	static constexpr double kRadiusCompressionExponent = 0.60;

private:
	static constexpr double kMercurySemiMajorAxisAu = 0.387098;
	static constexpr double kMercuryOrbitRenderRadius = 12.0;
	// The Sun alone is display-capped: by the radius law it would be 8.4
	// units and crowd Mercury's orbit. It stays by far the largest body.
	static constexpr double kSunRenderRadius = 6.0;
	static constexpr double kHeliocentricCompressionExponent = 0.55;
	static constexpr double kMoonOrbitCompressionExponent = 0.50;
	static constexpr double kMoonOrbitBaseInParentRadii = 1.60;
	static constexpr double kSpacecraftUnitsPerMetre = 0.002;
};

#endif
