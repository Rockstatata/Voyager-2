#include "Application.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "../rendering/Mesh.h"
#include "../rendering/UvSphereGenerator.h"
#include "../scene/BodyCatalog.h"
#include "../scene/EnvironmentBuilder.h"
#include "../scene/ScaleManager.h"
#include "../scene/SolarSystemBuilder.h"
#include "../scene/VoyagerModelBuilder.h"

Application::Application() = default;
Application::~Application() = default;

bool Application::initialize(int argc, char** argv)
{
	if (!m_window.create(1440, 900, "Voyager 2 Explorer"))
		return false;

	m_input.attach(m_window.handle());

	if (!m_renderer.initialize() || !m_text.initialize())
		return false;

	buildScene();

	std::cout << "[APP] initialized. F1: full control list. Free camera: C, then WASD + mouse (RMB or M), "
				 "Q/E or Ctrl/Space down/up, wheel speed, Shift fast, Alt fine. Tab: fly to a body. "
				 "1-6: mission bookmarks. V: Historical/Manual. Esc quits." << std::endl;

	for (int i = 1; i + 1 < argc; ++i)
	{
		const std::string option = argv[i];
		if (option == "--capture")
			startCaptureTour(argv[i + 1], "tour");
		else if (option == "--capture-bodies")
			startCaptureTour(argv[i + 1], "bodies");
		else if (option == "--capture-shading")
			startCaptureTour(argv[i + 1], "shading");
	}

	m_initialized = true;
	return true;
}

void Application::buildScene()
{
	m_sphereMesh = std::make_shared<Mesh>(UvSphereGenerator::generate(32, 64));

	SolarSystemBuilder::build(m_solarSystem,
		BodyCatalog::loadBodies("assets/data/celestial_bodies.csv"),
		BodyCatalog::loadRingBands("assets/data/ring_bands.csv"),
		m_sphereMesh, m_sunPosition);

	m_mission.load(m_solarSystem, m_sunPosition, BodyCatalog::loadBookmarks("assets/data/mission_bookmarks.csv"));

	VoyagerModelBuildResult model = VoyagerModelBuilder::build();
	m_voyager = model.spacecraft.get();
	// 13 m magnetometer boom tip to RTG boom tip, with margin.
	m_voyager->setBoundingRadius(ScaleManager().spacecraftSizeToRenderUnits(9.0));
	m_scene.group("spacecraft").addChild(std::move(model.spacecraft));
	m_mission.attachVoyager(m_voyager);
	m_cameraController.setVoyager(m_voyager);
	std::cout << "[VOYAGER] procedural spacecraft built (" << model.visiblePartCount
			  << " visible assemblies, " << model.renderedTriangleCount
			  << " rendered triangles), Historical mode" << std::endl;

	m_mission.buildTrajectory(m_scene.group("mission_path"));
	m_scene.group("mission_path").setVisible(false);

	m_backgroundLayers = EnvironmentBuilder::buildStarLayers();
	EnvironmentBuilder::buildOrbitGuides(m_scene.group("orbit_guides"), m_mission.ephemeris(), m_sunPosition);
	EnvironmentBuilder::buildHeliosphere(m_scene.group("heliosphere"), m_sunPosition);
	EnvironmentBuilder::buildSmallBodyFields(m_scene.group("small_bodies"), m_sunPosition);
	EnvironmentBuilder::buildComet(m_scene.group("comet"), m_sunPosition, m_sphereMesh);
	std::cout << "[SCENE] groups:";
	for (const auto& root : m_scene.roots())
		std::cout << ' ' << root->name() << '(' << root->children().size() << ')';
	std::cout << std::endl;

	m_mission.placeBodies();
	m_mission.placeVoyager(true);
	m_cameraController.enterChase();
}

void Application::run()
{
	if (!m_initialized)
	{
		std::cout << "[APP] run() called before a successful initialize()" << std::endl;
		return;
	}

	while (!m_window.shouldClose())
	{
		m_time.beginFrame(glfwGetTime());
		m_input.update();

		update(m_time.deltaTime());
		render();

		if (m_screenshotRequested)
		{
			m_screenshotRequested = false;
			std::filesystem::create_directories("captures");
			std::ostringstream path;
			path << "captures/screenshot_" << std::setw(3) << std::setfill('0') << ++m_screenshotCounter << ".bmp";
			if (saveScreenshot(path.str(), m_window.width(), m_window.height()))
				std::cout << "[APP] screenshot saved: " << path.str() << std::endl;
		}
		if (m_captureTour.active() &&
			!m_captureTour.afterRender(m_time.deltaTime(), m_window.width(), m_window.height()))
			m_window.requestClose();

		m_window.swapBuffers();
		m_window.pollEvents();
	}

	std::cout << "[APP] shutting down after " << m_time.frameCount() << " frames" << std::endl;
}

void Application::handleKeys()
{
	if (m_input.keyPressed(GLFW_KEY_ESCAPE))
		m_window.requestClose();
	if (m_input.keyPressed(GLFW_KEY_F1))
		m_helpVisible = !m_helpVisible;
	if (m_input.keyPressed(GLFW_KEY_F2))
		m_hudVisible = !m_hudVisible;
	if (m_input.keyPressed(GLFW_KEY_F12))
		m_screenshotRequested = true;

	if (m_input.keyPressed(GLFW_KEY_C))
	{
		if (m_cameraController.mode() == CameraController::Mode::FreeFly)
			m_cameraController.enterChase();
		else
			m_cameraController.enterFreeFly();
	}
	if (m_input.keyPressed(GLFW_KEY_HOME) || m_input.keyPressed(GLFW_KEY_H))
		m_cameraController.goToOverview();
	if (m_input.keyPressed(GLFW_KEY_TAB))
	{
		const bool backward = m_input.keyDown(GLFW_KEY_LEFT_SHIFT) || m_input.keyDown(GLFW_KEY_RIGHT_SHIFT);
		m_cameraController.focusNext(backward ? -1 : 1);
	}
	if (m_input.keyPressed(GLFW_KEY_G))
		m_cameraController.refocus();
	if (m_input.keyPressed(GLFW_KEY_M))
		m_cameraController.toggleMouseLook();

	if (m_input.keyPressed(GLFW_KEY_L))
		m_labelsVisible = !m_labelsVisible;
	m_lighting.handleKeys(m_input);
	if (m_input.keyPressed(GLFW_KEY_O))
	{
		SceneObject& guides = m_scene.group("orbit_guides");
		guides.setVisible(!guides.visible());
	}
	if (m_input.keyPressed(GLFW_KEY_T))
	{
		SceneObject& path = m_scene.group("mission_path");
		path.setVisible(!path.visible());
		std::cout << "[TRAJECTORY] line " << (path.visible() ? "shown" : "hidden") << std::endl;
	}

	SimulationClock& clock = m_mission.clock();
	if (m_input.keyPressed(GLFW_KEY_N))
	{
		clock.setEncounterSlowdown(!clock.encounterSlowdown());
		std::cout << "[APP] encounter slow-motion " << (clock.encounterSlowdown() ? "on" : "off") << std::endl;
	}
	if (m_input.keyPressed(GLFW_KEY_P))
	{
		clock.setPaused(!clock.paused());
		std::cout << "[APP] simulation " << (clock.paused() ? "paused" : "resumed") << std::endl;
	}
	if (m_input.keyPressed(GLFW_KEY_EQUAL) || m_input.keyPressed(GLFW_KEY_RIGHT_BRACKET) ||
		m_input.keyPressed(GLFW_KEY_KP_ADD))
		m_simulationSpeed = std::min(m_simulationSpeed * 2.0, 64.0);
	if (m_input.keyPressed(GLFW_KEY_MINUS) || m_input.keyPressed(GLFW_KEY_LEFT_BRACKET) ||
		m_input.keyPressed(GLFW_KEY_KP_SUBTRACT))
		m_simulationSpeed = std::max(m_simulationSpeed / 2.0, 1.0 / 64.0);
	if (m_input.keyPressed(GLFW_KEY_BACKSPACE))
	{
		m_simulationSpeed = 1.0;
		clock.setPaused(false);
	}

	if (m_input.keyPressed(GLFW_KEY_V))
	{
		const bool wasManual = m_voyager->flightMode() == Voyager2::FlightMode::Manual;
		m_voyager->setFlightMode(wasManual ? Voyager2::FlightMode::Historical : Voyager2::FlightMode::Manual);
		if (!wasManual && m_cameraController.mode() != CameraController::Mode::Chase)
			m_cameraController.enterChase();
		if (wasManual)
			m_mission.placeVoyager(true);
		std::cout << "[VOYAGER] flight mode: " << (wasManual ? "Historical" : "Manual") << std::endl;
	}

	for (int key = 0; key < 9; ++key)
	{
		if (m_input.keyPressed(GLFW_KEY_1 + key))
			jumpToBookmark(key);
	}
}

void Application::jumpToBookmark(int index)
{
	if (m_mission.jumpToBookmark(index) != nullptr)
		m_cameraController.enterChase();
}

void Application::update(double deltaTime)
{
	handleKeys();

	CelestialBody::setSimulationTimeScale(m_mission.clock().paused() ? 0.0 : m_simulationSpeed);

	// Manual piloting only when the chase camera owns the keyboard; in free
	// flight the same keys move the camera instead.
	const bool piloting = m_voyager->flightMode() == Voyager2::FlightMode::Manual &&
		m_cameraController.mode() == CameraController::Mode::Chase;
	if (piloting)
		m_voyager->applyManualControl(m_input, deltaTime);

	m_scene.update(deltaTime);
	m_mission.update(deltaTime, m_simulationSpeed);
	m_cameraController.update(m_input, deltaTime, piloting, m_captureTour.active());

	m_titleRefreshTimer += deltaTime;
	if (m_titleRefreshTimer >= 0.5)
	{
		refreshWindowTitle();
		m_titleRefreshTimer = 0.0;
	}
}

void Application::refreshWindowTitle()
{
	m_window.setTitle("Voyager 2 Explorer | " + HudOverlay::formatJulianDate(m_mission.clock().julianDate()) +
		" | F1: controls");
}

void Application::render()
{
	m_renderer.setLighting(m_lighting.build(m_sunPosition, m_camera,
		m_cameraController.nearestSurfaceDistance(m_camera.position())));
	m_renderer.beginFrame(m_camera, m_window.aspectRatio());
	for (const auto& layer : m_backgroundLayers)
		m_renderer.submitBackground(*layer->mesh(), *layer->material());
	m_scene.render(m_renderer);
	m_renderer.endFrame();

	HudView view;
	view.camera = &m_camera;
	view.cameraController = &m_cameraController;
	view.system = &m_solarSystem;
	view.voyager = m_voyager;
	view.mission = &m_mission;
	view.simulationSpeed = m_simulationSpeed;
	view.width = m_window.width();
	view.height = m_window.height();
	view.aspectRatio = m_window.aspectRatio();
	view.labelsVisible = m_labelsVisible;
	view.hudVisible = m_hudVisible;
	view.helpVisible = m_helpVisible;
	view.extraLines = m_lighting.statusLines();
	m_hud.render(m_text, view);
}

void Application::startCaptureTour(const std::string& directory, const std::string& kind)
{
	std::vector<CaptureTour::Shot> shots;
	if (kind == "bodies")
	{
		// One Focus view per registered body, for the per-object documents.
		for (int i = 0; i < static_cast<int>(m_solarSystem.bodies().size()); ++i)
		{
			shots.push_back({ m_solarSystem.bodies()[i]->data().id + ".bmp", 2.6, [this, i]()
			{
				m_mission.clock().setPaused(true);
				m_cameraController.focusBody(i);
			} });
		}
		m_captureTour.start(directory, std::move(shots));
		return;
	}

	auto focusById = [this](const char* id)
	{
		const auto& bodies = m_solarSystem.bodies();
		for (int i = 0; i < static_cast<int>(bodies.size()); ++i)
		{
			if (bodies[i]->data().id == id)
				m_cameraController.focusBody(i);
		}
	};
	auto beforeEncounter = [this](int bookmark, const char* planet, double days)
	{
		jumpToBookmark(bookmark);
		m_mission.freezeBefore(planet, days);
	};
	auto pause = [this]() { m_mission.clock().setPaused(true); };

	if (kind == "shading")
	{
		// The same Moon and Voyager views under every technique and light.
		const std::array<ShadingTechnique, 5> techniques = {
			ShadingTechnique::Flat, ShadingTechnique::Gouraud, ShadingTechnique::Phong,
			ShadingTechnique::BlinnPhong, ShadingTechnique::Toon };
		const std::array<const char*, 5> names = { "flat", "gouraud", "phong", "blinn_phong", "toon" };
		for (std::size_t i = 0; i < techniques.size(); ++i)
		{
			const ShadingTechnique technique = techniques[i];
			shots.push_back({ std::string("earth_") + names[i] + ".bmp", i == 0 ? 3.0 : 0.4,
				[=, this]()
				{
					m_lighting.setTechnique(technique);
					if (i == 0)
					{
						pause();
						focusById("earth");
					}
				} });
		}
		for (std::size_t i = 0; i < techniques.size(); ++i)
		{
			const ShadingTechnique technique = techniques[i];
			shots.push_back({ std::string("voyager_") + names[i] + ".bmp", i == 0 ? 3.0 : 0.4,
				[=, this]()
				{
					if (i == 0)
						beforeEncounter(1, "jupiter", 0.25);
					m_lighting.setTechnique(technique);
				} });
		}
		shots.push_back({ "light_headlamp.bmp", 3.0, [=, this]()
		{
			m_lighting.setTechnique(ShadingTechnique::BlinnPhong);
			m_lighting.setHeadlamp(true);
			pause();
			focusById("moon");
		} });
		shots.push_back({ "light_fill.bmp", 0.6, [=, this]() { m_lighting.setHeadlamp(false); m_lighting.setFill(true); } });
		shots.push_back({ "light_falloff.bmp", 3.0, [=, this]()
		{
			m_lighting.setFill(false);
			m_lighting.setSunFalloff(true);
			m_cameraController.goToOverview();
		} });
		m_captureTour.start(directory, std::move(shots));
		return;
	}

	shots = {
		{ "01_overview.bmp", 1.4, [=, this]() { pause(); m_cameraController.goToOverview(); } },
		{ "02_launch_chase.bmp", 2.8, [=, this]() { jumpToBookmark(0); pause(); } },
		{ "03_jupiter_approach.bmp", 3.0, [=, this]() { beforeEncounter(1, "jupiter", 0.25); } },
		{ "04_saturn_approach.bmp", 3.0, [=, this]() { beforeEncounter(2, "saturn", 0.12); } },
		{ "05_uranus_approach.bmp", 3.0, [=, this]() { beforeEncounter(3, "uranus", 0.12); } },
		{ "06_neptune_approach.bmp", 3.0, [=, this]() { beforeEncounter(4, "neptune", 0.06); } },
		{ "07_heliopause.bmp", 2.8, [=, this]() { jumpToBookmark(5); pause(); } },
		{ "08_focus_earth.bmp", 3.5, [=, this]() { jumpToBookmark(4); pause(); focusById("earth"); } },
		{ "09_focus_jupiter.bmp", 3.5, [=, this]() { pause(); focusById("jupiter"); } },
		{ "10_focus_saturn.bmp", 3.5, [=, this]() { pause(); focusById("saturn"); } },
		{ "11_focus_uranus.bmp", 3.5, [=, this]() { pause(); focusById("uranus"); } },
		{ "12_help.bmp", 0.5, [this]() { m_helpVisible = true; } },
	};
	m_captureTour.start(directory, std::move(shots));
}
