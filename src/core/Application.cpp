#include "Application.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <functional>
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

	if (!m_renderer.initialize() || !m_text.initialize() || !m_rayTracer.initialize())
		return false;

	buildScene();

	std::cout << "[APP] initialized. F1: full control list. Click a world to fly there, I to inspect Voyager, "
				 "C free flight (WASD + RMB), Tab planets, [ ] moons, F3-F10 lighting and ray tracing. "
				 "1-6: mission bookmarks. V: Historical/Manual. Esc twice quits." << std::endl;

	for (int i = 1; i + 1 < argc; ++i)
	{
		const std::string option = argv[i];
		if (option == "--capture")
			startCaptureTour(argv[i + 1], "tour");
		else if (option == "--capture-bodies")
			startCaptureTour(argv[i + 1], "bodies");
		else if (option == "--capture-shading")
			startCaptureTour(argv[i + 1], "shading");
		else if (option == "--capture-raytrace")
			startCaptureTour(argv[i + 1], "raytrace");
		else if (option == "--capture-voyager")
			startCaptureTour(argv[i + 1], "voyager");
		else if (option == "--capture-demo")
			startCaptureTour(argv[i + 1], "demo");
		else if (option == "--capture-assessment")
			startCaptureTour(argv[i + 1], "assessment");
		else if (option == "--benchmark")
			startBenchmark(argv[i + 1]);
	}

	m_initialized = true;
	return true;
}

void Application::buildScene()
{
	m_sphereMesh = std::make_shared<Mesh>(UvSphereGenerator::generate(32, 64));
	m_sphereMeshMedium = std::make_shared<Mesh>(UvSphereGenerator::generate(16, 32));
	m_sphereMeshLow = std::make_shared<Mesh>(UvSphereGenerator::generate(8, 16));
	CelestialBody::setSphereLods(m_sphereMesh.get(), m_sphereMeshMedium.get(), m_sphereMeshLow.get());

	SolarSystemBuilder::build(m_solarSystem,
		BodyCatalog::loadBodies("assets/data/celestial_bodies.csv"),
		BodyCatalog::loadRingBands("assets/data/ring_bands.csv"),
		m_sphereMesh, m_sunPosition);

	std::vector<std::string> texturePaths;
	for (const CelestialBody* body : m_solarSystem.bodies())
		texturePaths.push_back(body->data().texturePath);
	m_rayTracer.setTexturePaths(std::move(texturePaths));

	m_mission.load(m_solarSystem, m_sunPosition, BodyCatalog::loadBookmarks("assets/data/mission_bookmarks.csv"));

	VoyagerModelBuildResult model = VoyagerModelBuilder::build();
	m_voyager = model.spacecraft.get();
	m_voyagerBvh = model.traceMesh;
	m_voyagerBvh->build();
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
		m_benchmark.beginGpu();
		render();
		m_benchmark.endGpu();

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
		if (m_benchmark.active() && !m_benchmark.afterFrame(m_time.deltaTime(), m_renderer.stats()))
			m_window.requestClose();

		m_window.swapBuffers();
		m_window.pollEvents();
	}

	std::cout << "[APP] shutting down after " << m_time.frameCount() << " frames" << std::endl;
}

void Application::handleKeys()
{
	// Esc backs out first (help, then Inspect); quitting needs a second
	// press within two seconds, so a stray key cannot end a demonstration.
	if (m_input.keyPressed(GLFW_KEY_ESCAPE))
	{
		if (m_helpVisible)
			m_helpVisible = false;
		else if (m_cameraController.mode() == CameraController::Mode::Inspect)
			m_cameraController.enterChase();
		else if (m_quitArmedSeconds > 0.0)
			m_window.requestClose();
		else
			m_quitArmedSeconds = 2.0;
	}
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
	if (m_input.keyPressed(GLFW_KEY_RIGHT_BRACKET) || m_input.keyPressed(GLFW_KEY_PAGE_DOWN))
		m_cameraController.focusMoon(1);
	if (m_input.keyPressed(GLFW_KEY_LEFT_BRACKET) || m_input.keyPressed(GLFW_KEY_PAGE_UP))
		m_cameraController.focusMoon(-1);

	// Click to select: a world flies into Focus, Voyager opens Inspect.
	if (m_input.mouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT) && !m_input.cursorCaptured())
	{
		const glm::dvec2 mouse = m_input.mousePosition();
		const CameraController::Pick pick = m_cameraController.pickAt(mouse.x, mouse.y,
			m_window.width(), m_window.height(), m_window.aspectRatio());
		if (pick.voyager)
			m_cameraController.inspect(-1);
		else if (pick.bodyIndex >= 0)
			m_cameraController.focusBody(pick.bodyIndex);
	}
	if (m_input.keyPressed(GLFW_KEY_G))
		m_cameraController.refocus();
	if (m_input.keyPressed(GLFW_KEY_M))
		m_cameraController.toggleMouseLook();
	if (m_input.keyPressed(GLFW_KEY_I))
	{
		if (m_cameraController.mode() == CameraController::Mode::Inspect)
			m_cameraController.enterChase();
		else
			m_cameraController.inspect(-1);
	}
	if (m_cameraController.mode() == CameraController::Mode::Inspect)
	{
		if (m_input.keyPressed(GLFW_KEY_PERIOD))
			m_cameraController.inspectNext(1);
		if (m_input.keyPressed(GLFW_KEY_COMMA))
			m_cameraController.inspectNext(-1);
	}

	if (m_input.keyPressed(GLFW_KEY_L))
		m_labelsVisible = !m_labelsVisible;
	m_lighting.handleKeys(m_input);
	if (m_input.keyPressed(GLFW_KEY_F9))
	{
		m_rayTraced = !m_rayTraced;
		std::cout << "[APP] render mode: " << (m_rayTraced ? "ray-traced" : "raster") << std::endl;
	}
	if (m_input.keyPressed(GLFW_KEY_F10))
		m_rayTracer.setMaxBounces(m_rayTracer.maxBounces() == 0 ? 1 : 0);
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
	if (m_input.keyPressed(GLFW_KEY_EQUAL) || m_input.keyPressed(GLFW_KEY_KP_ADD))
		m_simulationSpeed = std::min(m_simulationSpeed * 2.0, 64.0);
	if (m_input.keyPressed(GLFW_KEY_MINUS) || m_input.keyPressed(GLFW_KEY_KP_SUBTRACT))
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

void Application::startBenchmark(const std::string& outputPath)
{
	auto focusById = [this](const char* id)
	{
		const auto& bodies = m_solarSystem.bodies();
		for (int i = 0; i < static_cast<int>(bodies.size()); ++i)
		{
			if (bodies[i]->data().id == id)
				m_cameraController.focusBody(i);
		}
	};
	auto pause = [this]() { m_mission.clock().setPaused(true); };
	auto raster = [this]() { m_rayTraced = false; };

	// The same views every run, covering each expensive path: the whole
	// system, Voyager close up (self-shadowing), ringed planets (shadow
	// rays through rings) and the ray-traced view.
	std::vector<Benchmark::View> views = {
		{ "overview", [=, this]() { raster(); pause(); m_cameraController.goToOverview(); } },
		{ "launch_chase", [=, this]() { raster(); jumpToBookmark(0); pause(); } },
		{ "jupiter_focus", [=, this]() { raster(); pause(); focusById("jupiter"); } },
		{ "saturn_focus", [=, this]() { raster(); pause(); focusById("saturn"); } },
		{ "voyager_inspect", [=, this]() { raster(); jumpToBookmark(0); pause(); m_cameraController.inspect(-1); } },
		{ "voyager_dish_closeup", [=, this]() { raster(); m_cameraController.inspect(1); } },
		{ "saturn_raytraced", [=, this]() { pause(); focusById("saturn"); m_rayTraced = true; } },
		{ "voyager_raytraced", [=, this]() { jumpToBookmark(0); pause(); m_cameraController.inspect(-1); m_rayTraced = true; } },
	};
	m_window.setVsync(false);
	m_benchmark.start(outputPath, std::move(views));
}

void Application::jumpToBookmark(int index)
{
	if (m_mission.jumpToBookmark(index) != nullptr)
		m_cameraController.enterChase();
}

void Application::update(double deltaTime)
{
	m_quitArmedSeconds = std::max(0.0, m_quitArmedSeconds - deltaTime);
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
	if (m_demoOrbit)
	{
		// The assessment tour moves the eye and its attached spotlight together.
		// This uses the existing camera/light seam; normal interactive rigs are unchanged.
		m_demoOrbitSeconds += deltaTime;
		if (const CelestialBody* body = m_cameraController.focusedBody())
		{
			const glm::dmat4 world = body->worldMatrix();
			const double radius = glm::length(glm::dvec3(world[0]));
			const glm::dvec3 target(world[3]);
			const double angle = m_demoOrbitSeconds * 0.3;
			m_camera.setPosition(target + radius * 2.8 * glm::dvec3(std::sin(angle), 0.3, std::cos(angle)));
			m_camera.lookAt(target);
		}
	}

	m_titleRefreshTimer += deltaTime;
	if (m_titleRefreshTimer >= 0.5)
	{
		refreshWindowTitle();
		m_titleRefreshTimer = 0.0;
	}
}

std::string Application::keyHints() const
{
	// One line of the keys that matter in the current situation; F1 has all.
	switch (m_cameraController.mode())
	{
		case CameraController::Mode::FreeFly:
			return "WASD FLY  SPACE/CTRL UP/DOWN  RMB LOOK  WHEEL SPEED  CLICK A WORLD TO FLY THERE  C CHASE  F1 ALL KEYS";
		case CameraController::Mode::Focus:
			return "RMB ORBIT  WHEEL ZOOM  TAB NEXT PLANET  [ ] MOONS  WASD FREE FLIGHT  C CHASE  F1 ALL KEYS";
		case CameraController::Mode::Inspect:
			return ", . NEXT COMPONENT  RMB ORBIT  WHEEL ZOOM  ESC OR I BACK TO CHASE  F9 RAY-TRACE";
		default:
			break;
	}
	if (m_voyager->flightMode() == Voyager2::FlightMode::Manual)
		return "W/S THRUST  A/D YAW  R/F PITCH  Q/E ROLL  X BRAKE  SHIFT BOOST  V HISTORICAL  I INSPECT";
	return "1-6 MISSION  P PAUSE  = - SPEED  V PILOT  I INSPECT VOYAGER  RMB ORBIT  WHEEL ZOOM  F1 ALL KEYS";
}

void Application::refreshWindowTitle()
{
	m_window.setTitle("Voyager 2 Explorer | " + HudOverlay::formatJulianDate(m_mission.clock().julianDate()) +
		" | F1: controls");
}

void Application::render()
{
	m_lighting.setInspectionFill(m_cameraController.mode() == CameraController::Mode::Inspect);
	m_renderer.setLighting(m_lighting.build(m_sunPosition, m_camera,
		m_cameraController.nearestSurfaceDistance(m_camera.position())));
	m_solarSystem.buildTraceScene(m_traceScene, m_sunPosition);
	m_renderer.setTraceScene(m_traceScene);
	m_rayTracer.setTracedMesh(m_voyagerBvh.get(), m_voyager->worldMatrix(), m_voyager->boundingRadius());
	m_renderer.beginFrame(m_camera, m_window.aspectRatio());

	// Voyager's self-shadows: a depth pass from the Sun, only while the craft
	// is on screen and big enough (within 200 bounding radii) to show them.
	const double voyagerRadius = m_voyager->boundingRadius();
	const glm::dvec3 voyagerPosition = m_voyager->transform().position;
	if (!m_rayTraced && m_renderer.isVisible(voyagerPosition, voyagerRadius) &&
		glm::length(voyagerPosition - m_camera.position()) < voyagerRadius * 200.0)
	{
		m_shadowCasters.clear();
		for (const auto& part : m_voyager->children())
		{
			if (part->mesh() != nullptr && part->visible())
				m_shadowCasters.push_back({ part->mesh().get(), part->worldMatrix() });
		}
		m_renderer.renderShadowMap(m_shadowCasters, voyagerPosition, voyagerRadius, m_sunPosition,
			m_window.width(), m_window.height());
	}
	for (const auto& layer : m_backgroundLayers)
		m_renderer.submitBackground(*layer->mesh(), *layer->material());

	// Ray-traced view: the bodies group (spheres, rings, Sun glow) is left
	// out of the raster pass and traced instead; everything else is still
	// rasterised and composites with it through the depth buffer.
	SceneObject& bodies = m_scene.group("bodies");
	SceneObject& spacecraft = m_scene.group("spacecraft");
	bodies.setVisible(!m_rayTraced);
	spacecraft.setVisible(!m_rayTraced);   // traced as triangles instead
	m_scene.render(m_renderer);
	bodies.setVisible(true);
	spacecraft.setVisible(true);
	if (m_rayTraced)
		m_rayTracer.render(m_camera, m_window.aspectRatio(), m_traceScene, m_renderer.lighting(), Renderer::kFarPlane);
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
	if (m_mission.beyondData())
		view.extraLines.push_back("BEYOND NASA/JPL DATA (2030): TWO-BODY KEPLER PREDICTION");
	view.hints = keyHints();
	if (m_quitArmedSeconds > 0.0)
		view.notice = "PRESS ESC AGAIN TO QUIT";
	else if (!m_demoCaption.empty())
	{
		view.notice = m_demoCaption;
		view.hints.clear(); // the chapter caption owns the bottom line during the tour
	}
	if (m_cameraController.mode() == CameraController::Mode::Inspect)
	{
		const int component = m_cameraController.inspectedComponent();
		const auto& components = m_voyager->components();
		if (component < 0)
		{
			view.caption = "VOYAGER 2";
			view.captionDetail = "722 KG, LAUNCHED 1977-08-20.  , AND . STEP THROUGH " +
				std::to_string(components.size()) + " COMPONENTS";
		}
		else
		{
			view.caption = components[component].name + "  (" + std::to_string(component + 1) + "/" +
				std::to_string(components.size()) + ")";
			view.captionDetail = components[component].description;
		}
	}
	view.extraLines.push_back(m_rayTraced
		? std::string("RENDER RAY-TRACED (F9)  REFLECTIONS ") + (m_rayTracer.maxBounces() > 0 ? "ON" : "OFF") + " (F10)"
		: std::string("RENDER RASTER (F9 RAY-TRACE)"));
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
	if (kind == "assessment")
	{
		// Fixed views expose construction, moving lights and dynamic objects.
		// All screenshots use the real scene; no extra renderable is created.
		m_hudVisible = false;
		m_labelsVisible = false;
		auto fixedView = [this](const glm::dvec3& target, const glm::dvec3& offset)
		{
			m_cameraController.enterFreeFly();
			m_camera.setPosition(target + offset);
			m_camera.lookAt(target);
		};
		auto moonView = [=, this]()
		{
			pause();
			const glm::dmat4 world = m_solarSystem.find("moon")->worldMatrix();
			const glm::dvec3 target(world[3]);
			const double radius = glm::length(glm::dvec3(world[0]));
			const glm::dvec3 towardSun = glm::normalize(m_sunPosition - target);
			const glm::dvec3 side = glm::normalize(glm::cross(towardSun, glm::dvec3(0.0, 1.0, 0.0)));
			fixedView(target, radius * 2.8 * glm::normalize(-towardSun + side * 0.7));
		};
		shots.push_back({ "moon_point.bmp", 0.5, [=, this]() { moonView(); } });
		shots.push_back({ "moon_spot.bmp", 0.5, [this]() { m_lighting.setHeadlamp(true); } });
		shots.push_back({ "moon_directional.bmp", 0.5, [this]()
		{
			m_lighting.setHeadlamp(false);
			m_lighting.setFill(true);
		} });
		shots.push_back({ "spot_move_0.bmp", 0.5, [=, this]()
		{
			m_lighting.setFill(false);
			m_lighting.setHeadlamp(true);
			moonView();
			m_demoOrbitSeconds = 0.0;
			m_demoOrbit = true;
			focusById("moon");
		} });
		shots.push_back({ "spot_move_1.bmp", 1.5, []() {} });
		shots.push_back({ "spot_move_2.bmp", 1.5, []() {} });
		shots.push_back({ "earth_wire.bmp", 2.8, [=, this]()
		{
			m_demoOrbit = false;
			m_lighting.setHeadlamp(false);
			focusById("earth");
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		} });
		shots.push_back({ "dish_wire.bmp", 2.8, [this]() { m_cameraController.inspect(1); } });
		shots.push_back({ "voyager_clean.bmp", 2.8, [this]()
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			m_scene.group("orbit_guides").setVisible(false);
			m_scene.group("small_bodies").setVisible(false);
			m_cameraController.inspect(-1);
		} });
		const std::array<ShadingTechnique, 5> techniques = { ShadingTechnique::Flat, ShadingTechnique::Gouraud,
			ShadingTechnique::Phong, ShadingTechnique::BlinnPhong, ShadingTechnique::Toon };
		const std::array<const char*, 5> names = { "flat", "gouraud", "phong", "blinn_phong", "toon" };
		for (std::size_t i = 0; i < techniques.size(); ++i)
		{
			shots.push_back({ std::string("voyager_clean_") + names[i] + ".bmp", 0.4,
				[this, technique = techniques[i]]() { m_lighting.setTechnique(technique); } });
		}
		shots.push_back({ "dish_clean.bmp", 2.8, [this]()
		{
			m_lighting.setTechnique(ShadingTechnique::BlinnPhong);
			m_cameraController.inspect(1);
		} });
		shots.push_back({ "comet.bmp", 0.6, [=, this]()
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			m_scene.group("small_bodies").setVisible(true);
			m_lighting.setFill(true);
			const glm::dvec3 target(m_scene.group("comet").children().front()->worldMatrix()[3]);
			const glm::dvec3 away = glm::normalize(target - m_sunPosition);
			const glm::dvec3 side = glm::normalize(glm::cross(away, glm::dvec3(0.0, 1.0, 0.0)));
			fixedView(target + away * 4.0, side * 17.0 + glm::dvec3(0.0, 4.0, 0.0));
		} });
		shots.push_back({ "asteroid_field.bmp", 0.6, [=, this]()
		{
			fixedView(m_sunPosition + glm::dvec3(34.0, 0.0, 0.0), glm::dvec3(0.0, 0.6, 2.0));
		} });
		shots.push_back({ "kuiper_field.bmp", 0.6, [=, this]()
		{
			fixedView(m_sunPosition + glm::dvec3(150.0, 0.0, 0.0), glm::dvec3(0.0, 2.0, 8.0));
		} });
		shots.push_back({ "oort_field.bmp", 0.6, [=, this]()
		{
			fixedView(m_sunPosition, glm::dvec3(0.0, 400.0, 2500.0));
		} });
		shots.push_back({ "jupiter_motion_0.bmp", 3.0, [=, this]()
		{
			focusById("jupiter");
			pause();
		} });
		shots.push_back({ "jupiter_motion_1.bmp", 1.2, [this]()
		{
			m_simulationSpeed = 1.0;
			m_mission.clock().setPaused(false);
		} });
		shots.push_back({ "jupiter_motion_2.bmp", 1.2, []() {} });
		shots.push_back({ "earth_spin_0.bmp", 3.0, [=, this]() { pause(); focusById("earth"); } });
		shots.push_back({ "earth_spin_1.bmp", 1.2, [this]()
		{
			m_simulationSpeed = 4.0;
			m_mission.clock().setPaused(false);
		} });
		shots.push_back({ "earth_spin_2.bmp", 1.2, []() {} });
		m_captureTour.start(directory, std::move(shots));
		return;
	}
	if (kind == "demo")
	{
		// 120 seconds of actual rendering, recorded externally to keep video
		// encoding and recording overhead out of the interactive application.
		auto chapter = [&](std::string name, double seconds, std::string caption, std::function<void()> setup)
		{
			shots.push_back({ std::string(name) + ".bmp", seconds, [=, this]()
			{
				m_demoOrbit = false;
				m_demoCaption = caption;
				setup();
			} });
		};
		chapter("01_overview", 8.0, "VOYAGER 2 - INTERACTIVE COMPUTER GRAPHICS", [=, this]()
		{
			pause();
			m_scene.group("mission_path").setVisible(true);
			m_cameraController.goToOverview();
		});
		chapter("02_objects", 10.0, "26 TEXTURED BODIES - SHARED PROCEDURAL SPHERES", [=, this]()
		{
			m_scene.group("mission_path").setVisible(false);
			focusById("earth");
			m_mission.clock().setPaused(false);
			m_simulationSpeed = 1.0 / 64.0;
		});
		chapter("03_curved_dish", 10.0, "CURVED SURFACE - SELF-AUTHORED PARABOLIC ANTENNA", [=, this]()
		{
			jumpToBookmark(0);
			pause();
			m_cameraController.inspect(1);
		});
		chapter("04_textures", 10.0, "NASA TEXTURE ATLAS - GEOMETRY DEFINES THE SILHOUETTE", [this]()
		{
			m_cameraController.inspect(-1);
		});
		chapter("05_motion", 12.0, "COMPLEX MOTION - DATE-SYNCHRONIZED JUPITER ENCOUNTER", [=, this]()
		{
			beforeEncounter(1, "jupiter", 0.25);
			m_simulationSpeed = 1.0 / 64.0;
			m_mission.clock().setPaused(false);
		});
		chapter("06_point", 6.0, "POINT LIGHT - SUN ILLUMINATES THE DAY SIDE", [=, this]()
		{
			pause();
			focusById("moon");
		});
		chapter("07_spot", 6.0, "SPOTLIGHT - SOFT CONE ATTACHED TO THE CAMERA (F5)", [this]()
		{
			m_lighting.setHeadlamp(true);
		});
		chapter("08_moving_spot", 6.0, "MOVING LIGHT - CAMERA AND HEADLAMP ORBIT THE MOON", [this]()
		{
			m_demoOrbitSeconds = 0.0;
			m_demoOrbit = true;
		});
		chapter("09_directional", 6.0, "DIRECTIONAL LIGHT - COOL FILL ON THE NIGHT SIDE (F6)", [=, this]()
		{
			m_lighting.setHeadlamp(false);
			m_lighting.setFill(true);
			focusById("moon");
		});
		const std::array<ShadingTechnique, 5> techniques = { ShadingTechnique::Flat, ShadingTechnique::Gouraud,
			ShadingTechnique::Phong, ShadingTechnique::BlinnPhong, ShadingTechnique::Toon };
		for (const ShadingTechnique technique : techniques)
		{
			const std::string name = "10_shading_" + std::to_string(static_cast<int>(technique));
			const std::string caption = std::string("SHADING COMPARISON (F3) - ") + shadingTechniqueName(technique);
			chapter(name, 4.0, caption, [=, this]() { m_lighting.setTechnique(technique); });
		}
		chapter("11_shadows", 6.0, "SATURN - RAY-TRACED SPHERE AND RING SHADOWS (F4)", [=, this]()
		{
			m_lighting.setTechnique(ShadingTechnique::BlinnPhong);
			m_lighting.setFill(false);
			focusById("saturn");
		});
		chapter("12_raytrace", 10.0, "WHITTED RAY TRACING - VOYAGER TRIANGLES THROUGH A BVH (F9)", [=, this]()
		{
			jumpToBookmark(0);
			pause();
			m_cameraController.inspect(-1);
			m_rayTraced = true;
			m_rayTracer.setMaxBounces(0);
		});
		chapter("13_reflections", 4.0, "ONE REFLECTION BOUNCE - METAL AND FOIL (F10)", [this]()
		{
			m_rayTracer.setMaxBounces(1);
		});
		chapter("14_controls", 6.0, "THANK YOU - CAMERA, TIME AND SIX-DOF SPACECRAFT CONTROLS", [this]()
		{
			m_rayTraced = false;
			m_helpVisible = true;
		});
		m_captureTour.start(directory, std::move(shots));
		return;
	}

	if (kind == "voyager")
	{
		// The whole spacecraft, then every named component, in Inspect mode.
		shots.push_back({ "voyager_00_whole.bmp", 3.0, [this]()
		{
			jumpToBookmark(0);
			m_mission.clock().setPaused(true);
			m_cameraController.inspect(-1);
		} });
		for (int i = 0; i < static_cast<int>(m_voyager->components().size()); ++i)
		{
			std::string name = m_voyager->components()[i].name;
			std::replace(name.begin(), name.end(), ' ', '_');
			const std::string index = (i + 1 < 10 ? "0" : "") + std::to_string(i + 1);
			shots.push_back({ "voyager_" + index + "_" + name + ".bmp", 2.4, [this, i]() { m_cameraController.inspect(i); } });
		}
		m_captureTour.start(directory, std::move(shots));
		return;
	}

	if (kind == "raytrace")
	{
		// Matching raster / ray-traced pairs of the same frozen views.
		auto pair = [&](const std::string& name, double settle, std::function<void()> setup)
		{
			shots.push_back({ name + "_raster.bmp", settle, [=, this]() { m_rayTraced = false; setup(); } });
			shots.push_back({ name + "_traced.bmp", 0.5, [this]() { m_rayTraced = true; } });
		};
		pair("saturn", 3.0, [=, this]() { pause(); focusById("saturn"); });
		pair("jupiter_system", 3.0, [=, this]() { pause(); focusById("jupiter"); });
		pair("earth", 3.0, [=, this]() { pause(); focusById("earth"); });
		pair("voyager_jupiter", 3.0, [=, this]() { beforeEncounter(1, "jupiter", 0.25); });
		pair("voyager_saturn", 3.0, [=, this]() { beforeEncounter(2, "saturn", 0.12); });
		pair("overview", 1.5, [=, this]() { pause(); m_cameraController.goToOverview(); });
		pair("voyager_inspect", 3.0, [=, this]() { jumpToBookmark(0); pause(); m_cameraController.inspect(-1); });
		pair("voyager_dish", 3.0, [=, this]() { m_cameraController.inspect(1); });
		m_captureTour.start(directory, std::move(shots));
		return;
	}

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
		shots.push_back({ "maps_moon_on.bmp", 3.0, [=, this]()
		{
			m_lighting.setTechnique(ShadingTechnique::BlinnPhong);
			pause();
			focusById("moon");
		} });
		shots.push_back({ "maps_moon_off.bmp", 0.4, [=, this]() { m_lighting.setSurfaceMaps(false); } });
		shots.push_back({ "maps_earth_on.bmp", 3.0, [=, this]() { m_lighting.setSurfaceMaps(true); focusById("earth"); } });
		shots.push_back({ "maps_earth_off.bmp", 0.4, [=, this]() { m_lighting.setSurfaceMaps(false); } });
		shots.push_back({ "shadow_saturn_soft.bmp", 3.0, [=, this]()
		{
			m_lighting.setSurfaceMaps(true);
			m_lighting.setShadows(ShadowMode::Soft);
			focusById("saturn");
		} });
		shots.push_back({ "shadow_saturn_hard.bmp", 0.4, [=, this]() { m_lighting.setShadows(ShadowMode::Hard); } });
		shots.push_back({ "shadow_saturn_off.bmp", 0.4, [=, this]() { m_lighting.setShadows(ShadowMode::Off); } });
		shots.push_back({ "light_headlamp.bmp", 3.0, [=, this]()
		{
			m_lighting.setShadows(ShadowMode::Soft);
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
		// Past the NASA/JPL tables: planets on two-body orbits, 2044-01-02.
		{ "13_beyond_data.bmp", 1.4, [=, this]()
		{
			m_helpVisible = false;
			jumpToBookmark(5);
			m_mission.clock().setJulianDate(2467616.5);
			pause();
			m_cameraController.goToOverview();
		} },
	};
	m_captureTour.start(directory, std::move(shots));
}
