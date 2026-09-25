#ifndef HUD_OVERLAY_H
#define HUD_OVERLAY_H

#include <string>
#include <vector>

#include <glm/glm.hpp>

class Camera;
class CameraController;
class MissionController;
class SolarSystem;
class TextRenderer;
class Voyager2;

// Read-only view of the application state the overlay describes.
struct HudView
{
	const Camera* camera = nullptr;
	const CameraController* cameraController = nullptr;
	const SolarSystem* system = nullptr;
	const Voyager2* voyager = nullptr;
	const MissionController* mission = nullptr;
	double simulationSpeed = 1.0;
	int width = 1;
	int height = 1;
	float aspectRatio = 1.0f;
	bool labelsVisible = true;
	bool hudVisible = true;
	bool helpVisible = false;
	// Short state lines appended to the panel by other systems (lighting...).
	std::vector<std::string> extraLines;
	// Transient message shown bottom-left (e.g. "Press Esc again to quit").
	std::string notice;
};

// Everything drawn in screen space (bible section 39): body labels, the
// telemetry panel, the encounter banner and the F1 controls sheet. Draws
// into a TextRenderer; never changes simulation state.
class HudOverlay
{
public:
	void render(TextRenderer& text, const HudView& view);

	// "1979-07-09 22:29 UTC" from a Julian Date (Meeus, ch. 7).
	static std::string formatJulianDate(double julianDate);

private:
	void renderLabels(TextRenderer& text, const HudView& view, float pixel);
	void renderPanel(TextRenderer& text, const HudView& view, float pixel);
	void renderEncounterBanner(TextRenderer& text, const HudView& view, float pixel);
	void renderHelp(TextRenderer& text, const HudView& view, float pixel);

	glm::vec4 m_panelRect{ 0.0f }; // left, top, right, bottom in pixels
};

#endif
