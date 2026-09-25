#ifndef SURFACE_MAPS_H
#define SURFACE_MAPS_H

#include <vector>

// Lighting maps derived on the CPU from a body's albedo, so every map is
// made by this project and stays registered texel-for-texel with the credited
// photograph (docs/objects/lighting.md, "Lighting maps"). Pure pixel maths:
// no OpenGL here; MaterialLibrary uploads the results.
class SurfaceMaps
{
public:
	// Treats albedo luminance as height (bright highland, dark maria and
	// crater floors) and returns a tangent-space normal map in RGBA8:
	// n = normalize(-dh/du * strength, -dh/dv * strength, 1) * 0.5 + 0.5.
	// Longitude wraps (the sphere seam), latitude clamps at the poles.
	static std::vector<unsigned char> normalMapFromAlbedo(const std::vector<unsigned char>& albedo,
		int width, int height, float strength);

	// Water mask for Earth: blue-dominant, dark texels are ocean (specular
	// 1.0); land, ice and cloud get 0.12. Returned in the red channel.
	static std::vector<unsigned char> oceanSpecularMap(const std::vector<unsigned char>& albedo,
		int width, int height);
};

#endif
