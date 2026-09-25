#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>
#include <string>
#include <vector>

#include "CameraController.h"
#include "CaptureTour.h"
#include "Input.h"
#include "LightingController.h"
#include "Time.h"
#include "Window.h"
#include "../rendering/Camera.h"
#include "../rendering/Renderer.h"
#include "../rendering/TextRenderer.h"
#include "../scene/MissionController.h"
#include "../scene/Scene.h"
#include "../scene/SolarSystem.h"
#include "../scene/Voyager2.h"
#include "../ui/HudOverlay.h"

class Mesh;

// Top-level owner of the window, subsystems and loop (bible section 10).
// It composes the systems and routes input; the systems do the work:
//   MissionController  - date, ephemeris, Voyager historical placement
//   CameraController   - camera modes, focus and framing
//   HudOverlay         - labels, telemetry panel, help
//   CaptureTour        - scripted screenshots
// The loop keeps update and render strictly separate: update() may change
// scene state, render() may not.
class Application
{
public:
	Application();
	~Application();

	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	// Creates the window/context and loads GPU resources. `--capture <dir>`
	// runs the scripted screenshot tour and exits; `--capture-bodies <dir>`
	// shoots one Focus view per body and `--capture-shading <dir>` one view
	// per shading technique and light. Returns false if initialization failed.
	bool initialize(int argc = 0, char** argv = nullptr);

	void run();

private:
	void buildScene();
	void handleKeys();
	void update(double deltaTime);
	void render();
	void refreshWindowTitle();
	void jumpToBookmark(int index);
	// kind: "tour", "bodies" or "shading".
	void startCaptureTour(const std::string& directory, const std::string& kind);

	Window m_window;
	Input m_input;
	Time m_time;
	Camera m_camera;
	Renderer m_renderer;
	TextRenderer m_text;
	Scene m_scene;

	// One unit sphere shared by every body (bible F9).
	std::shared_ptr<Mesh> m_sphereMesh;

	// Scene groups. SolarSystem indexes the bodies inside `bodies`.
	SolarSystem m_solarSystem{ m_scene.group("bodies") };
	MissionController m_mission;
	CameraController m_cameraController{ m_camera, m_solarSystem, m_mission };
	LightingController m_lighting;
	RayTraceScene m_traceScene; // rebuilt every frame, reused storage
	HudOverlay m_hud;
	CaptureTour m_captureTour;

	Voyager2* m_voyager = nullptr;              // non-owning; in the `spacecraft` group
	std::vector<std::unique_ptr<SceneObject>> m_backgroundLayers; // camera-centred stars

	const glm::dvec3 m_sunPosition{ 0.0, 0.0, -30.0 };
	bool m_labelsVisible = true;
	bool m_hudVisible = true;
	bool m_helpVisible = false;
	bool m_screenshotRequested = false;
	int m_screenshotCounter = 0;
	double m_simulationSpeed = 1.0;
	double m_titleRefreshTimer = 0.0;
	bool m_initialized = false;
};

#endif
