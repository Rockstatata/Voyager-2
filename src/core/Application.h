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
#include "../scene/Scene.h"
#include "../scene/SolarSystem.h"
#include "../scene/Trajectory.h"
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

	// Creates the window/context and loads GPU resources.
	// Returns false if the window, GLAD or the shader failed.
	bool initialize(int width = 800, int height = 800,
					const std::string& title = "Voyager 2 Explorer");

	void run();

private:
	// Bible section 35 "mode switching": 'C' cycles which of these drives
	// the view; 'V' (handled inside Voyager2) toggles Historical/Manual.
	// Focus is bible section 40's "object selection and focus" — minimal
	// version: keyboard cycling + camera snap, no on-screen labels/HUD
	// (that needs a font-rendering subsystem this project doesn't have).
	enum class CameraMode { FreeFly, ThirdPerson, Focus };

	bool loadShaders();
	void buildScene();
	void buildVoyager();
	void buildEnvironment(); // starfield, orbit rings, asteroid/Kuiper/Oort fields, and comets
	void updateHistoricalPlanetPositions(double julianDate);
	void update(double deltaTime);
	void render();
	void refreshWindowTitle();

	Window m_window;
	Input m_input;
	Time m_time;
	Camera m_camera;
	Renderer m_renderer;
	Scene m_scene;

	std::unique_ptr<Shader> m_shader;

	// One unit sphere is uploaded once and shared by every body — Sun,
	// planets and moons alike (bible F9: never one mesh upload per body).
	std::shared_ptr<Mesh> m_sphereMesh;

	// Owns nothing itself; inserts each CelestialBody into m_scene and
	// indexes it by id (bible section 18/52, Phase 3).
	SolarSystem m_solarSystem{ m_scene };

	Voyager2* m_voyager = nullptr; // non-owning; the Scene owns it
	SceneObject* m_voyagerTrajectory = nullptr; // non-owning; toggled with T
	struct HistoricalPlanetTrack
	{
		CelestialBody* body = nullptr;
		Trajectory trajectory;
		double referenceJulianDate = 0.0;
		double meanAnomalyAtReference = 0.0;
	};
	std::vector<HistoricalPlanetTrack> m_historicalPlanetTracks;
	SceneObject* m_starfield = nullptr; // non-owning; camera-centred in update()
	// Voyager is the main character of the submitted experience, so launch
	// in its chase view; C still cycles to FreeFly and body Focus modes.
	CameraMode m_cameraMode = CameraMode::ThirdPerson;
	int m_focusIndex = -1; // index into m_solarSystem.bodies(); -1 = none focused yet
	bool m_trajectoryVisible = false;

	// Bible section 21, SimulationClock: pause/speed apply uniformly to
	// every CelestialBody's orbit + spin (CelestialBody::setSimulationTimeScale
	// is a shared static, so one call here reaches all of them) and Voyager's
	// historical playback. Manual input/camera remain on real dt.
	bool m_simulationPaused = false;
	double m_simulationTimeScale = 1.0;
	double m_titleRefreshTimer = 0.0;

	// Non-owning; the Scene owns them. Read each frame so the drifting
	// comet's tail can point away from the Sun dynamically instead of in a
	// fixed local direction.
	SceneObject* m_driftingComet = nullptr;
	SceneObject* m_driftingCometTail = nullptr;

	bool m_initialized = false;
};

#endif
