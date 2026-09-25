#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>
#include <string>
#include <vector>

#include "Input.h"
#include "Time.h"
#include "Window.h"
#include "../rendering/Camera.h"
#include "../rendering/Renderer.h"
#include "../rendering/TextRenderer.h"
#include "../scene/MissionEphemeris.h"
#include "../scene/Scene.h"
#include "../scene/SimulationClock.h"
#include "../scene/SolarSystem.h"
#include "../scene/Voyager2.h"

class Shader;
class Mesh;

// Top-level owner of the window, subsystems and loop (bible section 10).
// main() does nothing but construct this and call run().
//
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
	// runs the scripted screenshot tour used for documentation and visual
	// verification, then exits; `--capture-bodies <dir>` shoots one Focus view
	// per body. Returns false if initialization failed.
	bool initialize(int argc = 0, char** argv = nullptr);

	void run();

private:
	// FreeFly: fly anywhere. Chase: orbit rig on Voyager. Focus: orbit rig
	// on the selected body (bible sections 37 and 40).
	enum class CameraMode { FreeFly, Chase, Focus };

	struct PlanetBinding
	{
		CelestialBody* body = nullptr;
		const MissionEphemeris::Planet* ephemeris = nullptr;
	};

	struct CaptureShot
	{
		std::string fileName;
		double settleSeconds = 2.5; // real time: the fly-to transition lasts 1.6 s
		void (*setup)(Application&) = nullptr;
	};

	bool loadShaders();
	void buildScene();
	void buildBodies();
	void buildRings();
	void buildEphemeris();
	void buildVoyager();
	void buildEnvironment(); // starfield, orbit guides, heliosphere, small-body fields, comet

	void update(double deltaTime);
	void handleGlobalKeys();
	void updateEphemerisPositions();
	void updateVoyagerHistorical(bool snap);
	void updateComet();
	void updateCamera(double deltaTime);
	double nearestSurfaceDistance(const glm::dvec3& point) const;
	glm::dquat chaseFrame() const;
	static glm::dquat frameLookingAlong(const glm::dvec3& forward);
	void render();
	void renderOverlay();
	void renderLabels();
	void renderHud();
	void refreshWindowTitle();

	void goToOverview();
	void focusBody(int index);
	void enterChase();
	void jumpToBookmark(int index);
	void freezeBeforeEncounter(int bookmark, double daysBefore); // capture tour only
	bool saveScreenshot(const std::string& path) const;
	void setupCaptureTour(const std::string& directory, bool everyBody);

	Window m_window;
	Input m_input;
	Time m_time;
	Camera m_camera;
	Renderer m_renderer;
	TextRenderer m_text;
	Scene m_scene;

	std::unique_ptr<Shader> m_shader;

	// One unit sphere is uploaded once and shared by every body — Sun,
	// planets and moons alike (bible F9: never one mesh upload per body).
	std::shared_ptr<Mesh> m_sphereMesh;

	// Owns nothing itself; inserts each CelestialBody into m_scene and
	// indexes it by id (bible section 18/52, Phase 3).
	SolarSystem m_solarSystem{ m_scene };
	MissionEphemeris m_ephemeris;
	SimulationClock m_clock;
	std::vector<PlanetBinding> m_planetBindings;
	glm::dvec3 m_sunPosition{ 0.0, 0.0, -30.0 };

	Voyager2* m_voyager = nullptr;              // non-owning; the Scene owns it
	SceneObject* m_voyagerTrajectory = nullptr; // non-owning; toggled with T
	std::vector<SceneObject*> m_orbitGuides;    // non-owning; toggled with O
	// Camera-centred background layers, drawn before the scene without
	// depth writes. Owned here, not by the Scene, because they are not
	// world objects.
	std::vector<std::unique_ptr<SceneObject>> m_backgroundLayers;
	SceneObject* m_driftingComet = nullptr;
	SceneObject* m_driftingCometTail = nullptr;

	CameraMode m_cameraMode = CameraMode::Chase;
	int m_focusIndex = -1; // index into m_solarSystem.bodies(); -1 = none selected
	bool m_trajectoryVisible = false;
	bool m_orbitGuidesVisible = true;
	bool m_labelsVisible = true;
	bool m_hudVisible = true;
	bool m_helpVisible = false;
	bool m_mouseLookLatched = false;
	double m_simulationSpeed = 1.0;
	glm::vec4 m_hudPanelRect{ 0.0f }; // left, top, right, bottom in pixels

	// Scripted capture tour (--capture).
	std::vector<CaptureShot> m_captureShots;
	std::string m_captureDirectory;
	std::size_t m_captureIndex = 0;
	int m_captureCursor = 0;
	double m_captureSecondsRemaining = 0.0;
	int m_screenshotCounter = 0;
	bool m_screenshotRequested = false;

	double m_titleRefreshTimer = 0.0;
	bool m_initialized = false;
};

#endif
