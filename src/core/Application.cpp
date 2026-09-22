#include "Application.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/constants.hpp>

#include "../../shaderClass.h"
#include "../rendering/CircleGenerator.h"
#include "../rendering/CylinderGenerator.h"
#include "../rendering/Material.h"
#include "../rendering/Mesh.h"
#include "../rendering/RingGenerator.h"
#include "../rendering/StarfieldGenerator.h"
#include "../rendering/Texture2D.h"
#include "../rendering/UvSphereGenerator.h"
#include "../scene/BodyType.h"
#include "../scene/CelestialBodyData.h"
#include "../scene/InstancedField.h"
#include "../scene/ScaleManager.h"
#include "../scene/Trajectory.h"
#include "../scene/VoyagerModelBuilder.h"

#include <random>

namespace
{
	// Real-body facts for every body this phase requires (bible section 3/18:
	// Sun + all eight planets + the "important moons" list — Earth's Moon,
	// the four Galilean moons, Titan, the five major Uranian moons, Triton).
	// Values are standard NASA Planetary Fact Sheet approximations
	// (https://nssdc.gsfc.nasa.gov/planetary/factsheet/), not flight-grade
	// ephemeris data — sufficient for an educational scene, cited per-object
	// in docs/objects/. A negative rotationPeriodHours marks retrograde spin.
	std::vector<CelestialBodyData> buildBodySpecs()
	{
		return {
			// id, displayName, parentId, radiusKm, semiMajorAxisKm, eccentricity,
			// orbitalPeriodDays, rotationPeriodHours, axialTiltDegrees, texturePath,
			// materialId, type
			// --- Sun ---
			{ "sun", "Sun", "", 696340.0, 0.0, 0.0, 0.0, 587.28, 7.25,
			  "assets/textures/bodies/sun.jpg", "emissive", BodyType::Star },

			// --- Planets, in orbital order ---
			{ "mercury", "Mercury", "", 2439.7, 57909050.0, 0.2056, 87.969, 1407.6, 0.034,
			  "assets/textures/bodies/mercury.jpg", "surface", BodyType::Planet },
			{ "venus", "Venus", "", 6051.8, 108208000.0, 0.0068, 224.701, -5832.5, 177.4,
			  "assets/textures/bodies/venus.jpg", "surface", BodyType::Planet },
			{ "earth", "Earth", "", 6371.0, 149598023.0, 0.0167, 365.256, 23.9345, 23.44,
			  "assets/textures/bodies/earth.jpg", "surface", BodyType::Planet },
			{ "mars", "Mars", "", 3389.5, 227939200.0, 0.0934, 686.980, 24.6229, 25.19,
			  "assets/textures/bodies/mars.jpg", "surface", BodyType::Planet },
			{ "jupiter", "Jupiter", "", 69911.0, 778570000.0, 0.0489, 4332.59, 9.9250, 3.13,
			  "assets/textures/bodies/jupiter.jpg", "gas-giant", BodyType::Planet },
			{ "saturn", "Saturn", "", 58232.0, 1433530000.0, 0.0565, 10759.22, 10.656, 26.73,
			  "assets/textures/bodies/saturn.jpg", "gas-giant", BodyType::Planet },
			{ "uranus", "Uranus", "", 25362.0, 2872460000.0, 0.0464, 30688.5, 17.24, 97.77,
			  "assets/textures/bodies/uranus.jpg", "ice-giant", BodyType::Planet },
			{ "neptune", "Neptune", "", 24622.0, 4495060000.0, 0.0086, 60195.0, 16.11, 28.32,
			  "assets/textures/bodies/neptune.jpg", "ice-giant", BodyType::Planet },

			// --- Pluto (bible section 3: "recommended for completeness ...
			// even though Voyager 2 did not encounter it") ---
			{ "pluto", "Pluto", "", 1188.3, 5906440000.0, 0.2488, 90560.0, -153.3, 122.53,
			  "assets/textures/bodies/pluto.jpg", "surface", BodyType::DwarfPlanet },

			// --- Earth's Moon ---
			{ "moon", "Moon", "earth", 1737.4, 384400.0, 0.0549, 27.322, 655.7, 6.68,
			  "assets/textures/bodies/moon.jpg", "surface", BodyType::Moon },

			// --- Galilean moons of Jupiter, in real distance order ---
			{ "io", "Io", "jupiter", 1821.6, 421700.0, 0.0041, 1.769, 42.46, 0.0,
			  "assets/textures/bodies/io.jpg", "surface", BodyType::Moon },
			{ "europa", "Europa", "jupiter", 1560.8, 671100.0, 0.009, 3.551, 85.2, 0.1,
			  "assets/textures/bodies/europa.jpg", "surface", BodyType::Moon },
			{ "ganymede", "Ganymede", "jupiter", 2634.1, 1070400.0, 0.0013, 7.155, 171.7, 0.33,
			  "assets/textures/bodies/ganymede.jpg", "surface", BodyType::Moon },
			{ "callisto", "Callisto", "jupiter", 2410.3, 1882700.0, 0.0074, 16.69, 400.5, 0.0,
			  "assets/textures/bodies/callisto.jpg", "surface", BodyType::Moon },

			// --- Saturn's Titan, plus the four optional/"when scope permits"
			// moons the bible names (section 3: Rhea, Iapetus, Dione, Tethys),
			// in real distance order so their render-proximity siblingIndex
			// (see below) matches real relative order too ---
			{ "tethys", "Tethys", "saturn", 531.1, 294619.0, 0.0001, 1.888, 45.3, 0.0,
			  "assets/textures/bodies/tethys.jpg", "surface", BodyType::Moon },
			{ "dione", "Dione", "saturn", 561.4, 377396.0, 0.0022, 2.737, 65.7, 0.0,
			  "assets/textures/bodies/dione.jpg", "surface", BodyType::Moon },
			{ "rhea", "Rhea", "saturn", 763.8, 527108.0, 0.001, 4.518, 108.4, 0.0,
			  "assets/textures/bodies/rhea.jpg", "surface", BodyType::Moon },
			{ "titan", "Titan", "saturn", 2574.7, 1221870.0, 0.0288, 15.945, 382.7, 0.3,
			  "assets/textures/bodies/titan.jpg", "surface", BodyType::Moon },
			{ "iapetus", "Iapetus", "saturn", 734.5, 3560820.0, 0.0286, 79.32, 1903.7, 0.0,
			  "assets/textures/bodies/iapetus.jpg", "surface", BodyType::Moon },

			// --- Major moons of Uranus, in real distance order ---
			{ "miranda", "Miranda", "uranus", 235.8, 129900.0, 0.0013, 1.413, 33.9, 0.0,
			  "assets/textures/bodies/miranda.jpg", "surface", BodyType::Moon },
			{ "ariel", "Ariel", "uranus", 578.9, 190900.0, 0.0012, 2.520, 60.5, 0.0,
			  "assets/textures/bodies/ariel.jpg", "surface", BodyType::Moon },
			{ "umbriel", "Umbriel", "uranus", 584.7, 266000.0, 0.0039, 4.144, 99.5, 0.0,
			  "assets/textures/bodies/umbriel.jpg", "surface", BodyType::Moon },
			{ "titania", "Titania", "uranus", 788.4, 436300.0, 0.0011, 8.706, 208.9, 0.0,
			  "assets/textures/bodies/titania.jpg", "surface", BodyType::Moon },
			{ "oberon", "Oberon", "uranus", 761.4, 583500.0, 0.0014, 13.46, 323.1, 0.0,
			  "assets/textures/bodies/oberon.jpg", "surface", BodyType::Moon },

			// --- Neptune's Triton (retrograde orbit; unusual 157 degree tilt;
			// its real orbit is essentially circular, e~0) ---
			{ "triton", "Triton", "neptune", 1353.4, 354800.0, 0.00002, 5.877, 141.0, 157.0,
			  "assets/textures/bodies/triton.jpg", "surface", BodyType::Moon },
		};
	}

	std::shared_ptr<Material> loadMaterial(const std::string& texturePath, const glm::vec3& fallbackColor)
	{
		auto material = std::make_shared<Material>();
		material->baseColor = fallbackColor;

		auto texture = std::make_shared<Texture2D>();
		if (texture->loadFromFile(texturePath))
		{
			material->albedoTexture = std::move(texture);
			material->baseColor = glm::vec3(1.0f);
		}
		return material;
	}

	std::shared_ptr<Material> flatMaterial(const glm::vec3& color)
	{
		auto material = std::make_shared<Material>();
		material->baseColor = color;
		return material;
	}

}

Application::Application() = default;
Application::~Application() = default;

bool Application::initialize(int width, int height, const std::string& title)
{
	if (!m_window.create(width, height, title))
		return false;

	m_input.attach(m_window.handle());

	if (!loadShaders())
		return false;

	m_renderer.setShader(m_shader.get());

	m_camera.setPosition(glm::dvec3(0.0, 1.2, 6.0));
	m_camera.setYawPitch(-90.0f, -10.0f);
	m_camera.setPerspective(45.0f, 0.0001f, 350.0f);

	buildScene();

	std::cout << "[APP] initialized. RMB + WASD to fly, Space/Ctrl up-down, Shift boost. "
				 "C: toggle fixed ThirdPerson/FreeFly camera. H/Home: scientific overview. Tab/Shift+Tab: focus a body. "
				 "P: pause simulation. =/-: simulation speed. V: Historical/Manual, T: trajectory, "
				 "1-6: mission bookmarks "
				 "(Manual: A/D or arrows yaw, W/S thrust, Space/Ctrl vertical). Esc to quit." << std::endl;

	m_initialized = true;
	return true;
}

bool Application::loadShaders()
{
	// Shader reads its files relative to the working directory, which must be
	// the project root. get_file_contents throws a bare errno on a missing file.
	try
	{
		m_shader = std::make_unique<Shader>("default.vert", "default.frag");
	}
	catch (...)
	{
		std::cout << "[SHADER] failed to read default.vert / default.frag. "
					 "The working directory must be the project root." << std::endl;
		return false;
	}

	GLint linked = GL_FALSE;
	glGetProgramiv(m_shader->ID, GL_LINK_STATUS, &linked);
	if (linked == GL_FALSE)
	{
		char log[1024] = {};
		glGetProgramInfoLog(m_shader->ID, sizeof(log), nullptr, log);
		std::cout << "[SHADER] program link failed:\n" << log << std::endl;
		return false;
	}

	return true;
}

void Application::buildScene()
{
	const MeshData sphereData = UvSphereGenerator::generate(32, 64);
	m_sphereMesh = std::make_shared<Mesh>(sphereData);

	constexpr double kTwoPi = 6.283185307179586;
	const ScaleManager scaleManager;
	const glm::dvec3 kSunPosition(0.0, 0.0, -30.0);

	// Compressed orbital clocks (bible section 21, simulation-clock
	// placeholder). Both preserve real RELATIVE speed — Mercury really does
	// orbit ~685x faster than Neptune; Io really does orbit ~9.6x faster
	// than Oberon — only the absolute rate is sped up, and by a different
	// constant for planets vs moons, since their real periods differ by
	// 3-4 orders of magnitude; one shared constant would make either the
	// planets crawl or the moons blur past too fast to read as orbiting.
	constexpr double kPlanetDaysPerSecond = 15.0;
	constexpr double kMoonDaysPerSecond = 0.4;

	const std::vector<std::string> planetOrder = {
		"mercury", "venus", "earth", "mars", "jupiter", "saturn", "uranus", "neptune"
	};
	// J2000 true anomalies in degrees. These replace the old decorative
	// 45-degree spacing, so Mars and the other manual-mode ellipses begin at
	// physically meaningful phases.
	const std::array<double, 8> j2000TrueAnomaliesDegrees = {
		177.3, 51.0, 357.4, 23.4, 22.0, 312.3, 143.6, 255.8
	};

	int bodiesLoaded = 0;
	for (const CelestialBodyData& data : buildBodySpecs())
	{
		auto material = loadMaterial(data.texturePath, glm::vec3(0.6f));
		if (material->albedoTexture != nullptr)
			++bodiesLoaded;

		if (data.id == "sun")
		{
			CelestialBody& sun = m_solarSystem.addBody(data, m_sphereMesh, material);
			sun.transform().position = kSunPosition;
			sun.transform().scale = glm::dvec3(scaleManager.sunRadiusToRenderUnits());
			continue;
		}

		if (data.parentId.empty())
		{
			// A planet: orbits the Sun. Kept as a scene ROOT, deliberately
			// NOT a child of the Sun's CelestialBody — a child's local
			// position is composed through Parent.worldMatrix(), which
			// includes the PARENT'S OWN SCALE, so an absolute orbit radius
			// stored as a child's local position would get silently
			// multiplied by the Sun's render radius. Passing the Sun's
			// actual world position as this orbit's `center` gets the same
			// "moves together with the Sun" behavior without that
			// cascade — see the moon branch below for what happens when
			// this isn't accounted for.
			// Pluto isn't one of the 8 evenly-spaced slots (it's a bonus
			// optional body, bible section 3) — falls back to index 8,
			// which just means it starts at the same angle as Mercury; at
			// Pluto's actual orbit radius (far beyond Neptune) that's not a
			// visible collision.
			const auto it = std::find(planetOrder.begin(), planetOrder.end(), data.id);
			const std::size_t index = it != planetOrder.end()
				? static_cast<std::size_t>(std::distance(planetOrder.begin(), it)) : planetOrder.size();
			const double orbitRadius = scaleManager.distanceToRenderUnits(data.semiMajorAxisKm);
			const double initialAngle = index < j2000TrueAnomaliesDegrees.size()
				? glm::radians(j2000TrueAnomaliesDegrees[index]) : 0.0;
			const double angularVelocity = kTwoPi * kPlanetDaysPerSecond / data.orbitalPeriodDays;

			CelestialBody& body = m_solarSystem.addBody(data, m_sphereMesh, material);
			body.transform().scale = glm::dvec3(scaleManager.radiusToRenderUnits(data.radiusKm));
			body.setOrbit(orbitRadius, angularVelocity, initialAngle, kSunPosition, data.eccentricity);
		}
		else
		{
			// A moon: genuinely a child of its planet (bible section 18:
			// "Do not manually update the Moon's world coordinate from
			// scratch if it can naturally be represented relative to
			// Earth"). Its local position AND its own scale are composed
			// through the PARENT's worldMatrix, which includes the
			// parent's own scale — so both this moon's orbit radius and
			// its own render radius must be pre-divided by the parent's
			// render radius to cancel that multiplication back out.
			// Skipping this (an earlier version of this code did) doesn't
			// just mis-scale things: it leaves the Moon rendering about 12x
			// too close to Earth and about 8x too small — in practice,
			// buried inside Earth's own sphere and invisible, for every
			// moon of every planet at once.
			CelestialBody* parent = m_solarSystem.find(data.parentId);
			const double parentRenderRadius = parent != nullptr ? parent->transform().scale.x : 1.0;
			const double siblingIndex = parent != nullptr ? static_cast<double>(parent->children().size()) : 0.0;

			const double moonRenderRadius = scaleManager.radiusToRenderUnits(data.radiusKm);
			// Preserve the real parent-relative orbital order instead of placing
			// moons by a small sibling-index offset. The mapping has a ring-safe
			// minimum and enough radial separation for the enlarged display moons.
			const double desiredWorldOffset = scaleManager.moonOrbitDistanceToRenderUnits(
				data.semiMajorAxisKm, parent != nullptr ? parent->data().radiusKm : 1.0,
				parentRenderRadius);
			const double orbitRadiusLocal = desiredWorldOffset / parentRenderRadius;
			const double initialAngle = siblingIndex * (kTwoPi / 6.0);
			const double angularVelocity = kTwoPi * kMoonDaysPerSecond / data.orbitalPeriodDays;

			CelestialBody& body = m_solarSystem.addBody(data, m_sphereMesh, material);
			body.transform().scale = glm::dvec3(moonRenderRadius / parentRenderRadius);
			body.setOrbit(orbitRadiusLocal, angularVelocity, initialAngle, glm::dvec3(0.0), data.eccentricity);
		}
	}

	std::cout << "[SCENE] " << m_solarSystem.bodies().size() << " bodies share 1 sphere mesh ("
			  << m_sphereMesh->vertexCount() << " vertices, "
			  << m_sphereMesh->indexCount() / 3 << " triangles), "
			  << bodiesLoaded << " real texture maps loaded" << std::endl;

	// Each named ring is its own band, in units of the parent planet's own
	// radius (see RingGenerator.h) — real km bounds divided by the planet's
	// real radius. Bands are separate SceneObjects (not one annulus) so each
	// can have its own flat shade and so the gaps between them (the Cassini
	// Division, etc) are real geometry gaps, not a texture — this project
	// has no per-vertex color yet, only per-material flat color.
	struct RingBand { float innerUnits; float outerUnits; glm::vec3 color; };
	auto buildRingSystem = [this](const std::string& planetId, const std::vector<RingBand>& bands)
	{
		CelestialBody* planet = m_solarSystem.find(planetId);
		if (planet == nullptr)
			return;

		int index = 0;
		for (const RingBand& band : bands)
		{
			const MeshData ringData = RingGenerator::generate(band.innerUnits, band.outerUnits);
			auto ring = std::make_unique<SceneObject>(planetId + "_ring_" + std::to_string(index++));
			ring->setMesh(std::make_shared<Mesh>(ringData));
			ring->setMaterial(flatMaterial(band.color));
			planet->addChild(std::move(ring));
		}
	};

	// Saturn, radius 58,232 km (source: ring-system radii from standard
	// planetary science references). Real gap at the Cassini Division.
	buildRingSystem("saturn", {
		{ 1.149f, 1.280f, glm::vec3(0.58f, 0.55f, 0.48f) },  // D ring, faint
		{ 1.282f, 1.580f, glm::vec3(0.66f, 0.60f, 0.50f) },  // C ring
		{ 1.580f, 2.019f, glm::vec3(0.88f, 0.80f, 0.64f) },  // B ring, brightest/widest
		// 2.019-2.098: Cassini Division — a real gap, deliberately no band here
		{ 2.098f, 2.349f, glm::vec3(0.78f, 0.71f, 0.57f) },  // A ring
		{ 2.402f, 2.415f, glm::vec3(0.82f, 0.77f, 0.68f) },  // F ring, thin
	});

	// Uranus, radius 25,362 km. Its rings are individually much narrower and
	// darker (carbon-rich, not icy) than Saturn's — five of its thirteen
	// named rings, widely spaced, dark charcoal shades, gaps everywhere else.
	buildRingSystem("uranus", {
		{ 1.660f, 1.668f, glm::vec3(0.33f, 0.34f, 0.36f) },  // 6/5/4 ring group
		{ 1.760f, 1.768f, glm::vec3(0.36f, 0.37f, 0.39f) },  // alpha ring
		{ 1.797f, 1.805f, glm::vec3(0.36f, 0.37f, 0.39f) },  // beta ring
		{ 1.874f, 1.882f, glm::vec3(0.38f, 0.39f, 0.41f) },  // gamma/eta/delta group
		{ 2.013f, 2.025f, glm::vec3(0.42f, 0.43f, 0.46f) },  // epsilon ring, widest/brightest
	});
	std::cout << "[SCENE] ring systems attached: Saturn (5 bands, Cassini Division gap), "
				 "Uranus (5 bands); Jupiter and Neptune ring geometry intentionally omitted" << std::endl;

	buildVoyager();
	buildEnvironment();
}

void Application::buildVoyager()
{
	VoyagerModelBuildResult model = VoyagerModelBuilder::build();
	auto voyager = std::move(model.spacecraft);
	m_voyager = voyager.get();

	const CelestialBody* sunBody = m_solarSystem.find("sun");
	const glm::dvec3 sunPosition = sunBody != nullptr ? sunBody->transform().position : glm::dvec3(0.0);
	glm::dvec3 startPosition = sunPosition + glm::dvec3(-2.0, 0.5, 1.5);

	// Load date-matched Earth, Mars, and giant-planet vectors before mapping Voyager's
	// display path. They share the same heliocentric frame, allowing the path
	// to preserve its real flyby direction while receiving a local visual
	// clearance (global logarithmic distance compression otherwise makes a
	// hundreds-of-thousands-km flyby look like a collision).
	m_historicalPlanetTracks.clear();
	for (const std::string& id : { "earth", "mars", "jupiter", "saturn", "uranus", "neptune" })
	{
		HistoricalPlanetTrack track;
		track.body = m_solarSystem.find(id);
		if (track.body != nullptr && track.trajectory.loadCsv(
			"assets/trajectory/planets/" + id + "_heliocentric.csv"))
		{
			const TrajectorySample& reference = track.trajectory.samples().front();
			const glm::dvec3 referenceSceneVector(
				reference.heliocentricAu.x, reference.heliocentricAu.z, reference.heliocentricAu.y);
			const double trueAnomaly = std::atan2(referenceSceneVector.z, referenceSceneVector.x);
			const double eccentricity = track.body->data().eccentricity;
			const double eccentricAnomaly = 2.0 * std::atan2(
				std::sqrt(1.0 - eccentricity) * std::sin(trueAnomaly * 0.5),
				std::sqrt(1.0 + eccentricity) * std::cos(trueAnomaly * 0.5));
			track.referenceJulianDate = reference.julianDate;
			track.meanAnomalyAtReference = eccentricAnomaly - eccentricity * std::sin(eccentricAnomaly);
			m_historicalPlanetTracks.push_back(std::move(track));
		}
	}

	Trajectory trajectory;
	if (trajectory.loadCsv("assets/trajectory/voyager2_heliocentric.csv"))
	{
		std::vector<glm::dvec3> renderPath = trajectory.buildRenderPath(ScaleManager(), sunPosition);
		if (renderPath.size() >= 2)
		{
			// The whole probe includes 10-13 m antenna/boom assemblies, not only
			// its root. Four target radii leaves a visibly unmistakable flyby
			// clearance for every component after educational scale compression.
			constexpr double kFlybyCenterDistanceInPlanetRadii = 4.00;
			auto applyVisualFlybyClearance = [&](const std::string& planetId, double julianDate)
			{
				auto pathPoint = std::find_if(trajectory.samples().begin(), trajectory.samples().end(),
					[julianDate](const TrajectorySample& sample)
					{
						return std::abs(sample.julianDate - julianDate) < 1e-6;
					});
				const auto track = std::find_if(m_historicalPlanetTracks.begin(), m_historicalPlanetTracks.end(),
					[&planetId](const HistoricalPlanetTrack& candidate)
					{
						return candidate.body->data().id == planetId;
					});
				if (pathPoint == trajectory.samples().end() || track == m_historicalPlanetTracks.end())
					return;

				const glm::dvec3 voyagerAu = pathPoint->heliocentricAu;
				const glm::dvec3 planetAu = track->trajectory.heliocentricPositionAtJulianDate(julianDate);
				const glm::dvec3 delta(voyagerAu.x - planetAu.x, voyagerAu.z - planetAu.z,
					voyagerAu.y - planetAu.y);
				const double deltaLength = glm::length(delta);
				if (deltaLength <= 1e-12)
					return;

				const std::size_t index = static_cast<std::size_t>(std::distance(trajectory.samples().begin(), pathPoint));
				const glm::dvec3 planetPosition = track->trajectory.renderPositionAtJulianDate(
					julianDate, ScaleManager(), sunPosition);
				renderPath[index] = planetPosition + glm::normalize(delta) *
					(track->body->transform().scale.x * kFlybyCenterDistanceInPlanetRadii);
			};

			applyVisualFlybyClearance("jupiter", 2444063.5);
			applyVisualFlybyClearance("saturn", 2444841.5);
			applyVisualFlybyClearance("uranus", 2446454.5);
			applyVisualFlybyClearance("neptune", 2447763.5);

			startPosition = renderPath.front();
			std::vector<double> julianDates;
			julianDates.reserve(trajectory.samples().size());
			for (const TrajectorySample& sample : trajectory.samples())
				julianDates.push_back(sample.julianDate);
			voyager->setHistoricalPath(renderPath, std::move(julianDates), 120.0);

			MeshData pathData;
			pathData.vertices.reserve(renderPath.size());
			pathData.indices.reserve(renderPath.size());
			for (std::size_t i = 0; i < renderPath.size(); ++i)
			{
				Vertex vertex;
				vertex.position = glm::vec3(renderPath[i]);
				vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
				vertex.texCoord = glm::vec2(
					static_cast<float>(i) / static_cast<float>(renderPath.size() - 1), 0.0f);
				pathData.vertices.push_back(vertex);
				pathData.indices.push_back(static_cast<std::uint32_t>(i));
			}

			auto pathObject = std::make_unique<SceneObject>("voyager2_historical_trajectory");
			pathObject->setMesh(std::make_shared<Mesh>(pathData, PrimitiveMode::LineStrip));
			pathObject->setMaterial(flatMaterial(glm::vec3(0.18f, 0.25f, 0.34f)));
			pathObject->setVisible(m_trajectoryVisible);
			m_voyagerTrajectory = pathObject.get();
			m_scene.addObject(std::move(pathObject));
		}
	}
	else
	{
		voyager->transform().position = startPosition;
	}

	m_scene.addObject(std::move(voyager));

	std::cout << "[VOYAGER] procedural spacecraft built (" << model.visiblePartCount
			  << " visible assemblies, " << model.renderedTriangleCount
			  << " rendered triangles; parabolic HGA, lattice booms, RTGs, science platform, "
				 "PWS antennas, 16 thrusters), Historical mode, starting at ("
			  << startPosition.x << ", " << startPosition.y << ", " << startPosition.z
			  << ")" << std::endl;
	std::cout << "[TRAJECTORY] historical planet tracks loaded: "
			  << m_historicalPlanetTracks.size() << std::endl;
	std::cout << "[TRAJECTORY] local flyby clearance: 4.00 target radii at Jupiter, Saturn, Uranus, Neptune"
			  << std::endl;
}

void Application::updateHistoricalPlanetPositions(double julianDate)
{
	if (m_historicalPlanetTracks.empty())
		return;

	const CelestialBody* sun = m_solarSystem.find("sun");
	const glm::dvec3 sunPosition = sun != nullptr ? sun->transform().position : glm::dvec3(0.0);
	for (const HistoricalPlanetTrack& track : m_historicalPlanetTracks)
	{
		const CelestialBodyData& data = track.body->data();
		if (data.orbitalPeriodDays <= 0.0)
			continue;

		// A date-anchored Kepler propagation gives a continuous velocity and
		// smooth curvature across the whole mission. Linear interpolation of
		// a handful of multi-year vectors made the bodies take visibly sharp
		// corners, despite the spacecraft and the real planets being in orbit.
		const double meanMotion = glm::two_pi<double>() / data.orbitalPeriodDays;
		const double meanAnomaly = track.meanAnomalyAtReference + meanMotion *
			(julianDate - track.referenceJulianDate);
		const double eccentricity = data.eccentricity;
		double eccentricAnomaly = meanAnomaly;
		for (int iteration = 0; iteration < 8; ++iteration)
		{
			const double residual = eccentricAnomaly - eccentricity * std::sin(eccentricAnomaly) - meanAnomaly;
			eccentricAnomaly -= residual / (1.0 - eccentricity * std::cos(eccentricAnomaly));
		}

		const double trueAnomaly = std::atan2(
			std::sqrt(1.0 - eccentricity * eccentricity) * std::sin(eccentricAnomaly),
			std::cos(eccentricAnomaly) - eccentricity);
		const double radius = ScaleManager().distanceToRenderUnits(data.semiMajorAxisKm) *
			(1.0 - eccentricity * std::cos(eccentricAnomaly));
		track.body->transform().position = sunPosition + radius * glm::dvec3(
			std::cos(trueAnomaly), 0.0, std::sin(trueAnomaly));
	}
}

namespace
{
	// Uniform point in a flat annulus (a belt), Y-jittered for thickness.
	// Fixed seed per field: deterministic, same reasoning as StarfieldGenerator.
	std::vector<glm::mat4> generateBeltMatrices(unsigned int count, float innerRadius, float outerRadius,
												 float heightJitter, float minScale, float maxScale,
												 const glm::dvec3& center, unsigned int seed)
	{
		std::mt19937 rng(seed);
		std::uniform_real_distribution<float> unit(0.0f, 1.0f);
		std::vector<glm::mat4> matrices;
		matrices.reserve(count);

		for (unsigned int i = 0; i < count; ++i)
		{
			const float theta = 2.0f * 3.14159265358979323846f * unit(rng);
			// sqrt spreads points evenly by AREA across the annulus, not by
			// radius — sampling radius linearly would bunch points near the
			// inner edge, the same non-uniform-density trap as sampling a
			// sphere's polar angle directly instead of cos(phi).
			const float radius = innerRadius + (outerRadius - innerRadius) * std::sqrt(unit(rng));
			const float y = (unit(rng) * 2.0f - 1.0f) * heightJitter;
			const glm::vec3 position(radius * std::cos(theta), y, radius * std::sin(theta));

			// Independent per-axis scale: a cheap stand-in for "irregular
			// shape" without new geometry — a perfect sphere stretched
			// unevenly on each axis no longer reads as a perfect sphere.
			const glm::vec3 scale(
				minScale + (maxScale - minScale) * unit(rng),
				minScale + (maxScale - minScale) * unit(rng),
				minScale + (maxScale - minScale) * unit(rng));

			glm::mat4 m(1.0f);
			m = glm::translate(m, glm::vec3(center) + position);
			m = glm::scale(m, scale);
			matrices.push_back(m);
		}
		return matrices;
	}

	// Uniform point in a spherical shell (Oort cloud) — same cos(phi)
	// reasoning as StarfieldGenerator, plus a radius sampled by volume
	// (cube root) so density doesn't bunch toward the inner edge.
	std::vector<glm::mat4> generateShellMatrices(unsigned int count, float innerRadius, float outerRadius,
												  float minScale, float maxScale,
												  const glm::dvec3& center, unsigned int seed)
	{
		std::mt19937 rng(seed);
		std::uniform_real_distribution<float> unit(0.0f, 1.0f);
		std::vector<glm::mat4> matrices;
		matrices.reserve(count);

		const float innerCubed = innerRadius * innerRadius * innerRadius;
		const float outerCubed = outerRadius * outerRadius * outerRadius;

		for (unsigned int i = 0; i < count; ++i)
		{
			const float theta = 2.0f * 3.14159265358979323846f * unit(rng);
			const float cosPhi = 2.0f * unit(rng) - 1.0f;
			const float sinPhi = std::sqrt(1.0f - cosPhi * cosPhi);
			const float radius = std::cbrt(innerCubed + (outerCubed - innerCubed) * unit(rng));

			const glm::vec3 position(radius * sinPhi * std::cos(theta),
									  radius * cosPhi,
									  radius * sinPhi * std::sin(theta));
			const glm::vec3 scale(
				minScale + (maxScale - minScale) * unit(rng),
				minScale + (maxScale - minScale) * unit(rng),
				minScale + (maxScale - minScale) * unit(rng));

			glm::mat4 m(1.0f);
			m = glm::translate(m, glm::vec3(center) + position);
			m = glm::scale(m, scale);
			matrices.push_back(m);
		}
		return matrices;
	}
}

void Application::buildEnvironment()
{
	// --- Background starfield: one Mesh, one GL_POINTS draw call. ---
	constexpr unsigned int kStarCount = 4000;
	constexpr float kStarfieldRadius = 80.0f;
	const MeshData starData = StarfieldGenerator::generate(kStarCount, kStarfieldRadius);
	auto starMesh = std::make_shared<Mesh>(starData, PrimitiveMode::Points);
	auto starfield = std::make_unique<SceneObject>("starfield");
	starfield->setMesh(starMesh);
	starfield->setMaterial(flatMaterial(glm::vec3(0.85f, 0.85f, 0.9f)));
	m_starfield = starfield.get();
	m_scene.addObject(std::move(starfield));

	// --- Orbit guides. Every planet uses the same smooth analytic Kepler
	// ellipse that defines its date-anchored historical propagation. Sparse
	// multi-year data chords are never displayed as crossing line clutter. ---
	const MeshData circleData = CircleGenerator::generate();
	auto circleMesh = std::make_shared<Mesh>(circleData, PrimitiveMode::LineLoop);
	CelestialBody* sun = m_solarSystem.find("sun");
	if (sun != nullptr)
	{
		for (CelestialBody* body : m_solarSystem.bodies())
		{
			if (body->data().type != BodyType::Planet)
				continue;
			constexpr unsigned int kEllipseSegments = 192;
			const double semiMajorAxis = ScaleManager().distanceToRenderUnits(body->data().semiMajorAxisKm);
			const double eccentricity = body->data().eccentricity;
			MeshData ellipseData;
			ellipseData.vertices.reserve(kEllipseSegments);
			ellipseData.indices.reserve(kEllipseSegments);
			for (unsigned int point = 0; point < kEllipseSegments; ++point)
			{
				const double trueAnomaly = glm::two_pi<double>() * static_cast<double>(point) /
					static_cast<double>(kEllipseSegments);
				const double radius = semiMajorAxis * (1.0 - eccentricity * eccentricity) /
					(1.0 + eccentricity * std::cos(trueAnomaly));
				Vertex vertex;
				vertex.position = glm::vec3(radius * std::cos(trueAnomaly), 0.0,
					radius * std::sin(trueAnomaly));
				vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
				vertex.texCoord = glm::vec2(static_cast<float>(point) / kEllipseSegments, 0.0f);
				ellipseData.vertices.push_back(vertex);
				ellipseData.indices.push_back(point);
			}
			auto orbitRing = std::make_unique<SceneObject>(body->data().id + "_orbit");
			orbitRing->setMesh(std::make_shared<Mesh>(ellipseData, PrimitiveMode::LineLoop));
			orbitRing->setMaterial(flatMaterial(glm::vec3(0.30f, 0.32f, 0.36f)));
			orbitRing->transform().position = sun->transform().position;
			m_scene.addObject(std::move(orbitRing));
		}
	}
	const glm::dvec3 sunPosition = sun != nullptr ? sun->transform().position : glm::dvec3(0.0);

	// --- Outer heliosphere markers. A sphere rendered as three orthogonal
	// great circles communicates a 3D boundary without a solid/transparent
	// shell that would hide the rest of this deliberately unlit scene. ---
	const ScaleManager scaleManager;
	constexpr double kKilometresPerAu = 149597870.7;
	auto addSphericalBoundary = [this, &circleMesh, &sunPosition](
		const std::string& name, double radius, const glm::vec3& color)
	{
		const std::array<glm::dquat, 3> rotations = {{
			glm::dquat(1.0, 0.0, 0.0, 0.0),
			glm::angleAxis(glm::radians(90.0), glm::dvec3(1.0, 0.0, 0.0)),
			glm::angleAxis(glm::radians(90.0), glm::dvec3(0.0, 0.0, 1.0)),
		}};
		for (std::size_t plane = 0; plane < rotations.size(); ++plane)
		{
			auto loop = std::make_unique<SceneObject>(name + "_plane_" + std::to_string(plane));
			loop->setMesh(circleMesh);
			loop->setMaterial(flatMaterial(color));
			loop->transform().position = sunPosition;
			loop->transform().rotation = rotations[plane];
			loop->transform().scale = glm::dvec3(radius);
			m_scene.addObject(std::move(loop));
		}
	};
	addSphericalBoundary("termination_shock",
		scaleManager.distanceToRenderUnits(94.0 * kKilometresPerAu), glm::vec3(0.12f, 0.22f, 0.30f));
	addSphericalBoundary("heliopause",
		scaleManager.distanceToRenderUnits(119.0 * kKilometresPerAu), glm::vec3(0.20f, 0.16f, 0.32f));

	// --- Small-body fields: each is ONE instanced draw call regardless of
	// count (bible failure mode F9). Same tiny low-poly rock mesh CPU data
	// reused for all three, but each field needs its OWN Mesh/GPU upload —
	// Mesh::setInstanceTransforms stores the instance buffer on the Mesh
	// itself, so one Mesh object cannot serve two different instanced
	// fields at once. ---
	const MeshData rockData = UvSphereGenerator::generate(6, 8); // low-poly: these are tiny/distant

	auto addInstancedField = [this, &rockData](const std::string& name, std::vector<glm::mat4> matrices,
												const glm::vec3& color)
	{
		auto mesh = std::make_shared<Mesh>(rockData);
		mesh->setInstanceTransforms(matrices);
		auto field = std::make_unique<InstancedField>(name);
		field->setMesh(mesh);
		field->setMaterial(flatMaterial(color));
		const std::size_t count = matrices.size();
		m_scene.addObject(std::move(field));
		std::cout << "[SCENE] " << name << ": " << count << " instances, 1 draw call" << std::endl;
	};

	// Radii tied to ScaleManager's log-compressed planet distances (Mars
	// ~5.44, Jupiter ~8.97, Neptune ~14.0 render units — see ScaleManager.h)
	// rather than fixed numbers, so the belts stay correctly positioned
	// relative to the planets even if those distance constants change.
	//
	// Asteroid belt: between Mars and Jupiter.
	const float asteroidInner = static_cast<float>(scaleManager.distanceToRenderUnits(2.1 * kKilometresPerAu));
	const float asteroidOuter = static_cast<float>(scaleManager.distanceToRenderUnits(3.3 * kKilometresPerAu));
	addInstancedField("asteroid_belt",
		generateBeltMatrices(2500, asteroidInner, asteroidOuter, 0.08f, 0.001f, 0.004f,
			sunPosition, 101),
		glm::vec3(0.45f, 0.42f, 0.38f));

	// Kuiper belt: beyond Neptune.
	const float kuiperInner = static_cast<float>(scaleManager.distanceToRenderUnits(30.0 * kKilometresPerAu));
	const float kuiperOuter = static_cast<float>(scaleManager.distanceToRenderUnits(50.0 * kKilometresPerAu));
	addInstancedField("kuiper_belt",
		generateBeltMatrices(1500, kuiperInner, kuiperOuter, 0.12f, 0.001f, 0.004f,
			sunPosition, 202),
		glm::vec3(0.55f, 0.55f, 0.60f));

	// Oort cloud: a sparse spherical shell far beyond everything else.
	const float oortInner = static_cast<float>(scaleManager.distanceToRenderUnits(2000.0 * kKilometresPerAu));
	const float oortOuter = static_cast<float>(scaleManager.distanceToRenderUnits(5000.0 * kKilometresPerAu));
	addInstancedField("oort_cloud",
		generateShellMatrices(800, oortInner, oortOuter, 0.002f, 0.006f, sunPosition, 303),
		glm::vec3(0.65f, 0.70f, 0.75f));

	// --- Drifting comet: a nucleus + a tapered tail, slowly orbiting far
	// beyond the Oort cloud. Reuses CelestialBody purely as a moving
	// container (bible section 22's orbit mechanism isn't specific to real
	// bodies) — its own Mesh/Material are null (SceneObject::render skips
	// drawing a null mesh, so the group itself is invisible; only its two
	// children draw), and its scale is left at identity specifically so
	// nucleus/tail can use absolute local sizes with no cascade correction
	// needed (see scale-manager.md — that correction is only necessary
	// when the parent's scale isn't 1). ---
	CelestialBodyData driftingCometData;
	driftingCometData.id = "drifting_comet";
	driftingCometData.displayName = "Drifting Comet";
	auto cometGroup = std::make_unique<CelestialBody>(driftingCometData, nullptr, nullptr);
	m_driftingComet = cometGroup.get();
	cometGroup->transform().scale = glm::dvec3(1.0);
	// Far beyond the Oort cloud shell (radius 25-40), very slow angular
	// speed (one revolution takes ~13 minutes) so it reads as "slowly
	// drifting" rather than orbiting like a planet. Initial angle points
	// it toward -Z, matching the default camera's starting look direction.
	cometGroup->setOrbit(scaleManager.distanceToRenderUnits(60.0 * kKilometresPerAu),
		0.008, 4.712389 /* 270 degrees */, sunPosition);

	auto cometNucleus = std::make_unique<SceneObject>("drifting_comet_nucleus");
	cometNucleus->setMesh(m_sphereMesh);
	cometNucleus->setMaterial(flatMaterial(glm::vec3(0.82f, 0.92f, 1.0f)));
	cometNucleus->transform().scale = glm::dvec3(0.02);
	cometGroup->addChild(std::move(cometNucleus));

	// Tapered cone (wide at the nucleus, a point at the far end) — the
	// "tail". Its rotation/position here are just a valid startup default;
	// Application::update() recomputes both every frame so the tail always
	// points directly away from the Sun as the comet moves (real comet
	// tails always point away from the Sun, not in a fixed direction —
	// see docs/objects/drifting-comet.md for that per-frame math).
	constexpr float kTailLength = 0.40f;
	const MeshData tailData = CylinderGenerator::generate(0.012f, 0.0f, kTailLength, 10);
	auto cometTail = std::make_unique<SceneObject>("drifting_comet_tail");
	cometTail->setMesh(std::make_shared<Mesh>(tailData));
	cometTail->setMaterial(flatMaterial(glm::vec3(0.78f, 0.94f, 1.0f))); // bright pale cyan: "shiny"
	cometTail->transform().rotation = glm::angleAxis(glm::radians(90.0), glm::dvec3(0.0, 0.0, 1.0));
	cometTail->transform().position = glm::dvec3(-kTailLength * 0.5, 0.0, 0.0);
	m_driftingCometTail = cometTail.get();
	cometGroup->addChild(std::move(cometTail));

	m_scene.addObject(std::move(cometGroup));

	std::cout << "[SCENE] environment built: camera-centred starfield (" << kStarCount << " stars), "
				 "9 smooth Kepler ellipse guides, termination-shock/heliopause wireframes, 3 small-body fields, "
				 "1 drifting comet with tail" << std::endl;
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

		m_window.swapBuffers();
		m_window.pollEvents();
	}

	std::cout << "[APP] shutting down after " << m_time.frameCount() << " frames" << std::endl;
}

void Application::update(double deltaTime)
{
	if (m_input.keyPressed(GLFW_KEY_ESCAPE))
		m_window.requestClose();

	if (m_input.keyPressed(GLFW_KEY_C))
	{
		// Direct fixed/free toggle. Focus is entered only with Tab, so camera
		// control is predictable during a demonstration.
		if (m_cameraMode == CameraMode::FreeFly) m_cameraMode = CameraMode::ThirdPerson;
		else m_cameraMode = CameraMode::FreeFly;
		const char* names[] = { "FreeFly", "ThirdPerson (Voyager 2)", "Focus" };
		std::cout << "[APP] camera mode: " << names[static_cast<int>(m_cameraMode)] << std::endl;
	}

	if (m_input.keyPressed(GLFW_KEY_HOME) || m_input.keyPressed(GLFW_KEY_H))
	{
		const CelestialBody* sun = m_solarSystem.find("sun");
		const glm::dvec3 sunPosition = sun != nullptr ? sun->transform().position : glm::dvec3(0.0);
		m_cameraMode = CameraMode::FreeFly;
		m_camera.setPosition(sunPosition + glm::dvec3(0.0, 25.0, 110.0));
		m_camera.setYawPitch(-90.0f, -12.8f);
		m_camera.setPerspective(45.0f, 0.0001f, 350.0f);
		std::cout << "[APP] scientific overview camera" << std::endl;
	}

	// Tab / Shift+Tab cycle the focused body and switch into Focus mode —
	// bible section 40's "object selection," minimal version (see
	// Application.h: no on-screen label/HUD, that needs a font-rendering
	// subsystem this project doesn't have).
	if (m_input.keyPressed(GLFW_KEY_TAB) && !m_solarSystem.bodies().empty())
	{
		const int count = static_cast<int>(m_solarSystem.bodies().size());
		const bool backward = m_input.keyDown(GLFW_KEY_LEFT_SHIFT) || m_input.keyDown(GLFW_KEY_RIGHT_SHIFT);
		m_focusIndex = ((m_focusIndex + (backward ? -1 : 1)) % count + count) % count;
		m_cameraMode = CameraMode::Focus;
		std::cout << "[APP] focused: " << m_solarSystem.bodies()[m_focusIndex]->data().displayName << std::endl;
	}

	if (m_input.keyPressed(GLFW_KEY_P))
	{
		m_simulationPaused = !m_simulationPaused;
		CelestialBody::setSimulationTimeScale(m_simulationPaused ? 0.0 : m_simulationTimeScale);
		std::cout << "[APP] simulation " << (m_simulationPaused ? "paused" : "resumed") << std::endl;
	}
	if (m_input.keyPressed(GLFW_KEY_EQUAL) || m_input.keyPressed(GLFW_KEY_RIGHT_BRACKET))
	{
		m_simulationTimeScale = std::min(m_simulationTimeScale * 1.5, 50.0);
		if (!m_simulationPaused) CelestialBody::setSimulationTimeScale(m_simulationTimeScale);
		std::cout << "[APP] simulation speed: " << m_simulationTimeScale << "x" << std::endl;
	}
	if (m_input.keyPressed(GLFW_KEY_MINUS) || m_input.keyPressed(GLFW_KEY_LEFT_BRACKET))
	{
		m_simulationTimeScale = std::max(m_simulationTimeScale / 1.5, 0.02);
		if (!m_simulationPaused) CelestialBody::setSimulationTimeScale(m_simulationTimeScale);
		std::cout << "[APP] simulation speed: " << m_simulationTimeScale << "x" << std::endl;
	}

	if (m_voyager != nullptr && m_input.keyPressed(GLFW_KEY_V))
	{
		const bool wasManual = m_voyager->flightMode() == Voyager2::FlightMode::Manual;
		m_voyager->setFlightMode(wasManual ? Voyager2::FlightMode::Historical : Voyager2::FlightMode::Manual);
		std::cout << "[VOYAGER] flight mode: " << (wasManual ? "Historical" : "Manual") << std::endl;
	}

	if (m_voyagerTrajectory != nullptr && m_input.keyPressed(GLFW_KEY_T))
	{
		m_trajectoryVisible = !m_trajectoryVisible;
		m_voyagerTrajectory->setVisible(m_trajectoryVisible);
		std::cout << "[TRAJECTORY] line " << (m_trajectoryVisible ? "shown" : "hidden") << std::endl;
	}

	if (m_voyager != nullptr)
	{
		struct Bookmark { int key; double julianDate; const char* name; };
		const std::array<Bookmark, 6> bookmarks = {{
			{ GLFW_KEY_1, 2443376.5, "Launch (1977-08-21)" },
			{ GLFW_KEY_2, 2444063.5, "Jupiter (1979-07-09)" },
			{ GLFW_KEY_3, 2444841.5, "Saturn (1981-08-25)" },
			{ GLFW_KEY_4, 2446454.5, "Uranus (1986-01-24)" },
			{ GLFW_KEY_5, 2447763.5, "Neptune (1989-08-25)" },
			{ GLFW_KEY_6, 2458427.5, "Interstellar space (2018-11-05)" },
		}};
		for (const Bookmark& bookmark : bookmarks)
		{
			if (!m_input.keyPressed(bookmark.key))
				continue;
			m_voyager->setFlightMode(Voyager2::FlightMode::Historical);
			m_voyager->setHistoricalJulianDate(bookmark.julianDate);
			m_cameraMode = CameraMode::ThirdPerson;
			std::cout << "[TRAJECTORY] bookmark: " << bookmark.name << std::endl;
			break;
		}
	}

	if (m_voyager != nullptr && m_voyager->flightMode() == Voyager2::FlightMode::Manual)
		m_voyager->applyManualControl(m_input, deltaTime);

	// Right mouse is consistently mouse-look in both free-fly and Voyager
	// orbit-follow modes. GLFW cursor capture prevents the cursor reaching a
	// screen edge and making camera orbit appear to stop or glitch.
	const bool mouseLookMode = m_cameraMode == CameraMode::FreeFly || m_cameraMode == CameraMode::ThirdPerson;
	m_input.setCursorCaptured(mouseLookMode && m_input.mouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT));

	if (m_cameraMode == CameraMode::FreeFly)
	{
		m_camera.setPerspective(45.0f, 0.0001f, 350.0f);
		m_camera.update(m_input, deltaTime);
	}

	if (m_voyager != nullptr)
		m_voyager->setHistoricalTimeScale(m_simulationPaused ? 0.0 : m_simulationTimeScale);

	m_scene.update(deltaTime);

	// Planetary ephemerides describe the currently selected historical date.
	// When the user takes manual control of Voyager, that date simply pauses.
	if (m_voyager != nullptr)
		updateHistoricalPlanetPositions(m_voyager->historicalJulianDate());

	// The drifting comet's tail always points away from the Sun, recomputed
	// every frame from the comet's current (moving) position — see
	// docs/objects/drifting-comet.md for the rotation-from-direction math.
	if (m_driftingComet != nullptr && m_driftingCometTail != nullptr)
	{
		const CelestialBody* sun = m_solarSystem.find("sun");
		if (sun != nullptr)
		{
			const glm::dvec3 awayFromSun = glm::normalize(
				m_driftingComet->transform().position - sun->transform().position);
			const glm::dvec3 yAxis(0.0, 1.0, 0.0);
			const glm::dvec3 axis = glm::cross(yAxis, awayFromSun);
			const double axisLength = glm::length(axis);
			glm::dquat rotation;
			if (axisLength < 1e-9)
				rotation = glm::dot(yAxis, awayFromSun) > 0.0
					? glm::dquat(1.0, 0.0, 0.0, 0.0)
					: glm::angleAxis(glm::pi<double>(), glm::dvec3(1.0, 0.0, 0.0));
			else
				rotation = glm::angleAxis(std::acos(glm::clamp(glm::dot(yAxis, awayFromSun), -1.0, 1.0)),
										   axis / axisLength);

			constexpr double kTailHalfLength = 1.5;
			m_driftingCometTail->transform().rotation = rotation;
			m_driftingCometTail->transform().position = rotation * glm::dvec3(0.0, kTailHalfLength, 0.0);
		}
	}

	if (m_cameraMode == CameraMode::ThirdPerson && m_voyager != nullptr)
	{
		// Directly behind local +Z (the flight heading), with enough distance
		// to keep the 13 m magnetometer boom and RTG assembly in frame.
		const ScaleManager scaleManager;
		const double kFollowDistance = scaleManager.spacecraftSizeToRenderUnits(80.0);
		const double kFollowHeight = scaleManager.spacecraftSizeToRenderUnits(20.0);
		// A 200-unit far plane includes the camera-centred star sphere, while
		// the 1e-5 near plane still keeps every procedural spacecraft detail.
		m_camera.setPerspective(45.0f, 1e-5f, 200.0f);
		m_camera.orbitFollowTarget(m_voyager->transform().position, m_voyager->headingForward(),
								kFollowDistance, kFollowHeight, m_input);
	}
	else if (m_cameraMode == CameraMode::Focus && m_focusIndex >= 0 &&
			 m_focusIndex < static_cast<int>(m_solarSystem.bodies().size()))
	{
		const CelestialBody* focused = m_solarSystem.bodies()[m_focusIndex];
		const glm::dmat4 world = focused->worldMatrix();
		const glm::dvec3 worldPosition(world[3]);
		// Length of the transformed X basis vector is this body's actual
		// world-space size — correct for a moon (a scene-graph CHILD, whose
		// own transform().scale is only LOCAL to its parent, not the final
		// world size) as well as a root planet, unlike reading
		// transform().scale directly.
		const double worldRadius = glm::length(glm::dvec3(world[0]));
		const double focusNearPlane = std::max(worldRadius * 0.001, 1e-8);
		const double focusFarPlane = std::max(worldRadius * 100.0, 1.0);
		m_camera.setPerspective(45.0f, static_cast<float>(focusNearPlane),
			static_cast<float>(focusFarPlane));
		const glm::dvec3 offsetDirection = glm::normalize(glm::dvec3(0.0, 0.35, 1.0));
		m_camera.followTarget(worldPosition, glm::vec3(-offsetDirection),
							   worldRadius * 4.0 + 0.05, worldRadius * 1.5);
	}

	// Star positions are camera-relative background geometry, so they remain
	// visible in every direction as Voyager crosses the large solar-system
	// scene instead of being left behind at the original world origin.
	if (m_starfield != nullptr)
		m_starfield->transform().position = m_camera.position();

	m_titleRefreshTimer += deltaTime;
	if (m_titleRefreshTimer >= 0.25)
	{
		refreshWindowTitle();
		m_titleRefreshTimer = 0.0;
	}
}

void Application::refreshWindowTitle()
{
	const char* cameraNames[] = { "FreeFly", "ThirdPerson", "Focus" };
	std::ostringstream title;
	title << "Voyager 2 Explorer | Camera: " << cameraNames[static_cast<int>(m_cameraMode)];
	if (m_voyager != nullptr)
	{
		const bool historical = m_voyager->flightMode() == Voyager2::FlightMode::Historical;
		title << " | Flight: " << (historical ? "Historical" : "Manual");
		if (historical)
			title << " | JD " << std::fixed << std::setprecision(1) << m_voyager->historicalJulianDate();
		const CelestialBody* sun = m_solarSystem.find("sun");
		const glm::dvec3 sunPosition = sun != nullptr ? sun->transform().position : glm::dvec3(0.0);
		title << " | Sun distance: " << std::fixed << std::setprecision(2)
			<< glm::length(m_voyager->transform().position - sunPosition) << "u"
			<< " | Speed: " << glm::length(m_voyager->velocity()) << "u/s";
	}
	title << " | Simulation: " << (m_simulationPaused ? "Paused" : "Running")
		<< " " << std::fixed << std::setprecision(2) << m_simulationTimeScale << "x";
	if (m_focusIndex >= 0 && m_focusIndex < static_cast<int>(m_solarSystem.bodies().size()))
		title << " | Focus: " << m_solarSystem.bodies()[m_focusIndex]->data().displayName;
	m_window.setTitle(title.str());
}

void Application::render()
{
	m_renderer.beginFrame(m_camera, m_window.aspectRatio());
	m_scene.render(m_renderer);
	m_renderer.endFrame();
}
