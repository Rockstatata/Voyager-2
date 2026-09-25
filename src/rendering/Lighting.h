#ifndef LIGHTING_H
#define LIGHTING_H

#include <array>

#include <glm/glm.hpp>

// How lighting is evaluated across a surface (the classic comparison):
//   Flat        - one normal per triangle (screen-space derivative), faceted.
//   Gouraud     - lighting computed per VERTEX, colour interpolated.
//   Phong       - normal interpolated, lighting per FRAGMENT, reflect(-l, n).
//   BlinnPhong  - per-fragment, specular from the half vector (default).
//   Toon        - per-fragment, diffuse quantised into bands, hard specular
//                 and a dark silhouette rim (cel shading).
enum class ShadingTechnique { Flat = 0, Gouraud = 1, Phong = 2, BlinnPhong = 3, Toon = 4 };

// Light casters. Positions are world space (double); the Renderer converts
// them to the camera-relative frame the shaders work in.
enum class LightType { Directional = 0, Point = 1, Spot = 2 };

// Ray-traced Sun shadows: off, hard (one ray to the Sun's centre) or soft
// (the Sun treated as a disc; partial cover gives a penumbra).
enum class ShadowMode { Off = 0, Hard = 1, Soft = 2 };

struct Light
{
	LightType type = LightType::Point;
	bool enabled = false;
	glm::dvec3 position{ 0.0 };           // point and spot
	glm::vec3 direction{ 0.0f, -1.0f, 0.0f }; // directional and spot: the way light travels
	glm::vec3 color{ 1.0f };
	float intensity = 1.0f;
	// 1 / (constant + linear d + quadratic d^2); (1, 0, 0) = no falloff.
	glm::vec3 attenuation{ 1.0f, 0.0f, 0.0f };
	float innerCutoffCos = 1.0f;          // spot: full intensity inside
	float outerCutoffCos = 1.0f;          // spot: zero outside, smooth between
};

constexpr int kMaxLights = 4;

struct LightingState
{
	bool enabled = true;
	ShadingTechnique technique = ShadingTechnique::BlinnPhong;
	float ambient = 0.07f;
	bool surfaceMaps = true; // normal and specular maps (F8)
	ShadowMode shadows = ShadowMode::Soft;
	// Light 0 is always the Sun; only it casts shadows.
	std::array<Light, kMaxLights> lights{};
};

inline const char* shadingTechniqueName(ShadingTechnique technique)
{
	switch (technique)
	{
		case ShadingTechnique::Flat: return "FLAT";
		case ShadingTechnique::Gouraud: return "GOURAUD";
		case ShadingTechnique::Phong: return "PHONG";
		case ShadingTechnique::Toon: return "TOON";
		default: return "BLINN-PHONG";
	}
}

#endif
