#include "Application.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
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

namespace
{
	constexpr double kKilometresPerAu = 149597870.7;
	constexpr double kOrbitGuideEpochJulianDate = 2447000.5; // 1987-07-15, mid-mission

	// Real-body facts (NASA Planetary Fact Sheet approximations,
	// https://nssdc.gsfc.nasa.gov/planetary/factsheet/). Planet positions come
	// from the Horizons ephemeris; semiMajorAxis/eccentricity/period remain
	// for moons, documentation and the HUD. Negative rotation = retrograde.
	std::vector<CelestialBodyData> buildBodySpecs()
	{
		return {
			// id, displayName, parentId, radiusKm, semiMajorAxisKm, eccentricity,
			// orbitalPeriodDays, rotationPeriodHours, axialTiltDegrees, texturePath,
			// materialId, type
			{ "sun", "Sun", "", 696340.0, 0.0, 0.0, 0.0, 587.28, 7.25,
			  "assets/textures/bodies/sun.jpg", "emissive", BodyType::Star },

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
			{ "pluto", "Pluto", "", 1188.3, 5906440000.0, 0.2488, 90560.0, -153.3, 122.53,
			  "assets/textures/bodies/pluto.jpg", "surface", BodyType::DwarfPlanet },

			{ "moon", "Moon", "earth", 1737.4, 384400.0, 0.0549, 27.322, 655.7, 6.68,
			  "assets/textures/bodies/moon.jpg", "surface", BodyType::Moon },

			{ "io", "Io", "jupiter", 1821.6, 421700.0, 0.0041, 1.769, 42.46, 0.0,
			  "assets/textures/bodies/io.jpg", "surface", BodyType::Moon },
			{ "europa", "Europa", "jupiter", 1560.8, 671100.0, 0.009, 3.551, 85.2, 0.1,
			  "assets/textures/bodies/europa.jpg", "surface", BodyType::Moon },
			{ "ganymede", "Ganymede", "jupiter", 2634.1, 1070400.0, 0.0013, 7.155, 171.7, 0.33,
			  "assets/textures/bodies/ganymede.jpg", "surface", BodyType::Moon },
			{ "callisto", "Callisto", "jupiter", 2410.3, 1882700.0, 0.0074, 16.69, 400.5, 0.0,
			  "assets/textures/bodies/callisto.jpg", "surface", BodyType::Moon },

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

			// Triton orbits Neptune retrograde: a negative period reverses it.
			{ "triton", "Triton", "neptune", 1353.4, 354800.0, 0.00002, -5.877, 141.0, 157.0,
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

	std::shared_ptr<Material> flatMaterial(const glm::vec3& color, ShadingModel shading = ShadingModel::Unlit,
		float opacity = 1.0f)
	{
		auto material = std::make_shared<Material>();
		material->baseColor = color;
		material->shading = shading;
		material->opacity = opacity;
		return material;
	}

	std::shared_ptr<Material> glowMaterial(const glm::vec3& color, float opacity, float falloff)
	{
		auto material = flatMaterial(color, ShadingModel::Glow, opacity);
		material->specularPower = falloff;
		return material;
	}

	std::shared_ptr<Mesh> polylineMesh(const std::vector<glm::dvec3>& points, const glm::dvec3& origin,
		PrimitiveMode mode)
	{
		MeshData data;
		data.vertices.reserve(points.size());
		data.indices.reserve(points.size());
		for (std::size_t i = 0; i < points.size(); ++i)
		{
			Vertex vertex;
			vertex.position = glm::vec3(points[i] - origin);
			vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
			vertex.texCoord = glm::vec2(static_cast<float>(i) / static_cast<float>(std::max<std::size_t>(points.size() - 1, 1)), 0.0f);
			data.vertices.push_back(vertex);
			data.indices.push_back(static_cast<std::uint32_t>(i));
		}
		return std::make_shared<Mesh>(data, mode);
	}

	// Julian Date (UT-like; the TDB offset of ~1 minute is below HUD
	// resolution) to a calendar string. Meeus, Astronomical Algorithms ch. 7.
	std::string formatJulianDate(double julianDate)
	{
		const double shifted = julianDate + 0.5;
		const double z = std::floor(shifted);
		double fraction = shifted - z;
		double a = z;
		if (z >= 2299161.0)
		{
			const double alpha = std::floor((z - 1867216.25) / 36524.25);
			a = z + 1.0 + alpha - std::floor(alpha / 4.0);
		}
		const double b = a + 1524.0;
		const double c = std::floor((b - 122.1) / 365.25);
		const double d = std::floor(365.25 * c);
		const double e = std::floor((b - d) / 30.6001);
		const int day = static_cast<int>(b - d - std::floor(30.6001 * e));
		const int month = static_cast<int>(e < 14.0 ? e - 1.0 : e - 13.0);
		const int year = static_cast<int>(month > 2 ? c - 4716.0 : c - 4715.0);
		int minutes = static_cast<int>(std::round(fraction * 1440.0));
		minutes = std::min(minutes, 1439);

		std::ostringstream text;
		text << year << '-' << std::setw(2) << std::setfill('0') << month << '-' << std::setw(2) << day
			<< ' ' << std::setw(2) << minutes / 60 << ':' << std::setw(2) << minutes % 60 << " UTC";
		return text.str();
	}

	std::string formatThousands(double value)
	{
		const long long rounded = static_cast<long long>(std::llround(value));
		std::string digits = std::to_string(std::llabs(rounded));
		for (int i = static_cast<int>(digits.size()) - 3; i > 0; i -= 3)
			digits.insert(static_cast<std::size_t>(i), ",");
		return (rounded < 0 ? "-" : "") + digits;
	}

	std::string fixed(double value, int precision)
	{
		std::ostringstream text;
		text << std::fixed << std::setprecision(precision) << value;
		return text.str();
	}

	// Uniform point in a flat annulus (a belt), Y-jittered for thickness.
	std::vector<glm::mat4> generateBeltMatrices(unsigned int count, float innerRadius, float outerRadius,
		float heightJitter, float minScale, float maxScale, const glm::dvec3& center, unsigned int seed)
	{
		std::mt19937 rng(seed);
		std::uniform_real_distribution<float> unit(0.0f, 1.0f);
		std::vector<glm::mat4> matrices;
		matrices.reserve(count);

		for (unsigned int i = 0; i < count; ++i)
		{
			const float theta = glm::two_pi<float>() * unit(rng);
			// sqrt spreads points evenly by AREA across the annulus.
			const float radius = innerRadius + (outerRadius - innerRadius) * std::sqrt(unit(rng));
			const float y = (unit(rng) * 2.0f - 1.0f) * heightJitter;
			const glm::vec3 position(radius * std::cos(theta), y, radius * std::sin(theta));
			// Independent per-axis scale: an irregular rock, not a perfect ball.
			const glm::vec3 scale(
				minScale + (maxScale - minScale) * unit(rng),
				minScale + (maxScale - minScale) * unit(rng),
				minScale + (maxScale - minScale) * unit(rng));
			const float spin = glm::two_pi<float>() * unit(rng);

			glm::mat4 m(1.0f);
			m = glm::translate(m, glm::vec3(center) + position);
			m = glm::rotate(m, spin, glm::normalize(glm::vec3(unit(rng) - 0.5f, 1.0f, unit(rng) - 0.5f)));
			m = glm::scale(m, scale);
			matrices.push_back(m);
		}
		return matrices;
	}

	// Uniform point in a spherical shell (Oort cloud), radius sampled by volume.
	std::vector<glm::mat4> generateShellMatrices(unsigned int count, float innerRadius, float outerRadius,
		float minScale, float maxScale, const glm::dvec3& center, unsigned int seed)
	{
		std::mt19937 rng(seed);
		std::uniform_real_distribution<float> unit(0.0f, 1.0f);
		std::vector<glm::mat4> matrices;
		matrices.reserve(count);

		const float innerCubed = innerRadius * innerRadius * innerRadius;
		const float outerCubed = outerRadius * outerRadius * outerRadius;
		for (unsigned int i = 0; i < count; ++i)
		{
			const float theta = glm::two_pi<float>() * unit(rng);
			const float cosPhi = 2.0f * unit(rng) - 1.0f;
			const float sinPhi = std::sqrt(1.0f - cosPhi * cosPhi);
			const float radius = std::cbrt(innerCubed + (outerCubed - innerCubed) * unit(rng));
			const glm::vec3 position(radius * sinPhi * std::cos(theta), radius * cosPhi,
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

	struct Bookmark
	{
		const char* name;
		const char* planetId; // empty = fixed date
		double julianDate;    // used when planetId is empty
	};

	// 2-5 start 1.5 days before closest approach: the encounter slow-motion
	// then carries Voyager through the whole flyby on screen.
	const std::array<Bookmark, 6> kBookmarks = {{
		{ "Launch (1977-08-21)", "", 0.0 },
		{ "Jupiter encounter (1979-07-09)", "jupiter", 0.0 },
		{ "Saturn encounter (1981-08-26)", "saturn", 0.0 },
		{ "Uranus encounter (1986-01-24)", "uranus", 0.0 },
		{ "Neptune encounter (1989-08-25)", "neptune", 0.0 },
		{ "Heliopause crossing (2018-11-05)", "", 2458427.5 },
	}};
	constexpr double kBookmarkLeadDays = 1.5;
}

Application::Application() = default;
Application::~Application() = default;

bool Application::initialize(int argc, char** argv)
{
	if (!m_window.create(1440, 900, "Voyager 2 Explorer"))
		return false;

	m_input.attach(m_window.handle());

	if (!loadShaders())
		return false;
	if (!m_text.initialize())
		return false;

	m_renderer.setShader(m_shader.get());

	buildScene();

	std::cout << "[APP] initialized. F1: full control list. Free camera: C, then WASD + mouse (RMB or M), "
				 "Q/E or Ctrl/Space down/up, wheel speed, Shift fast, Alt fine. Tab: fly to a body. "
				 "1-6: mission bookmarks. V: Historical/Manual. Esc quits." << std::endl;

	for (int i = 1; i + 1 < argc; ++i)
	{
		const std::string option = argv[i];
		if (option == "--capture" || option == "--capture-bodies")
			setupCaptureTour(argv[i + 1], option == "--capture-bodies");
	}

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

	buildBodies();
	buildRings();
	buildEphemeris();
	buildVoyager();
	buildEnvironment();

	updateEphemerisPositions();
	updateVoyagerHistorical(true);
	enterChase();
}

void Application::buildBodies()
{
	const ScaleManager scaleManager;
	// Moons keep a visual orbital clock (planet positions come from the
	// dated ephemeris). Real relative speeds are preserved: Io still laps
	// Callisto ~9.4 times.
	constexpr double kMoonDaysPerSecond = 0.4;

	int texturesLoaded = 0;
	for (const CelestialBodyData& data : buildBodySpecs())
	{
		auto material = loadMaterial(data.texturePath, glm::vec3(0.6f));
		if (material->albedoTexture != nullptr)
			++texturesLoaded;

		if (data.id == "sun")
		{
			material->shading = ShadingModel::Unlit; // the light source itself
			CelestialBody& sun = m_solarSystem.addBody(data, m_sphereMesh, material);
			sun.transform().position = m_sunPosition;
			sun.transform().scale = glm::dvec3(scaleManager.sunRadiusToRenderUnits());

			// Two additive fresnel shells: a tight bright corona and a wide
			// faint halo. Children inherit the Sun's scale (1.0 = one radius).
			auto corona = std::make_unique<SceneObject>("sun_corona");
			corona->setMesh(m_sphereMesh);
			corona->setMaterial(glowMaterial(glm::vec3(1.0f, 0.80f, 0.45f), 0.9f, 1.5f));
			corona->transform().scale = glm::dvec3(1.25);
			sun.addChild(std::move(corona));
			auto halo = std::make_unique<SceneObject>("sun_halo");
			halo->setMesh(m_sphereMesh);
			halo->setMaterial(glowMaterial(glm::vec3(1.0f, 0.60f, 0.25f), 0.55f, 3.0f));
			halo->transform().scale = glm::dvec3(2.6);
			sun.addChild(std::move(halo));
			continue;
		}

		if (data.id == "earth")
			material->specularStrength = 0.18f;
		else if (data.type == BodyType::Planet)
			material->specularStrength = 0.05f;

		if (data.parentId.empty())
		{
			// Planets are scene roots positioned every frame from the dated
			// Horizons ephemeris (updateEphemerisPositions).
			CelestialBody& body = m_solarSystem.addBody(data, m_sphereMesh, material);
			body.transform().scale = glm::dvec3(scaleManager.radiusToRenderUnits(data.radiusKm));
			continue;
		}

		// A moon is a child of its planet. Its local position and scale are
		// composed through the planet's world matrix, which includes the
		// planet's scale, so both are divided by the planet's render radius.
		CelestialBody* parent = m_solarSystem.find(data.parentId);
		const double parentRenderRadius = parent != nullptr ? parent->transform().scale.x : 1.0;
		const double siblingIndex = parent != nullptr ? static_cast<double>(parent->children().size()) : 0.0;
		const double moonRenderRadius = scaleManager.radiusToRenderUnits(data.radiusKm);
		const double orbitWorld = scaleManager.moonOrbitDistanceToRenderUnits(
			data.semiMajorAxisKm, parent != nullptr ? parent->data().radiusKm : 1.0, parentRenderRadius);
		// Golden-angle spacing spreads the starting phases of siblings.
		const double initialAngle = siblingIndex * 2.39996322972865332;
		const double angularVelocity = glm::two_pi<double>() * kMoonDaysPerSecond / data.orbitalPeriodDays;

		CelestialBody& body = m_solarSystem.addBody(data, m_sphereMesh, material);
		body.transform().scale = glm::dvec3(moonRenderRadius / parentRenderRadius);
		body.setOrbit(orbitWorld / parentRenderRadius, angularVelocity, initialAngle, glm::dvec3(0.0),
			data.eccentricity);
	}

	std::cout << "[SCENE] " << m_solarSystem.bodies().size() << " bodies share 1 sphere mesh ("
			  << m_sphereMesh->vertexCount() << " vertices, " << m_sphereMesh->indexCount() / 3
			  << " triangles), " << texturesLoaded << " real texture maps loaded" << std::endl;
}

void Application::buildRings()
{
	// Each named ring is its own band, in units of the parent planet's own
	// radius (RingGenerator.h): real km bounds divided by the planet's real
	// radius. Real gaps (Cassini Division etc.) are geometry gaps.
	struct RingBand { float innerUnits; float outerUnits; glm::vec3 color; float opacity; };
	auto buildRingSystem = [this](const std::string& planetId, const std::vector<RingBand>& bands)
	{
		CelestialBody* planet = m_solarSystem.find(planetId);
		if (planet == nullptr)
			return;

		int index = 0;
		for (const RingBand& band : bands)
		{
			const MeshData ringData = RingGenerator::generate(band.innerUnits, band.outerUnits, 128);
			auto ring = std::make_unique<SceneObject>(planetId + "_ring_" + std::to_string(index++));
			ring->setMesh(std::make_shared<Mesh>(ringData));
			ring->setMaterial(flatMaterial(band.color, ShadingModel::LitTwoSided, band.opacity));
			planet->addChild(std::move(ring));
		}
	};

	// Saturn, radius 58,232 km. Real gap at the Cassini Division.
	buildRingSystem("saturn", {
		{ 1.110f, 1.236f, glm::vec3(0.42f, 0.39f, 0.34f), 0.35f },  // D ring, faint
		{ 1.239f, 1.527f, glm::vec3(0.62f, 0.57f, 0.48f), 0.70f },  // C ring
		{ 1.527f, 1.951f, glm::vec3(0.93f, 0.85f, 0.69f), 0.97f },  // B ring, brightest/widest
		// 1.951-2.027: Cassini Division, deliberately no band
		{ 2.027f, 2.269f, glm::vec3(0.82f, 0.75f, 0.61f), 0.90f },  // A ring
		{ 2.320f, 2.334f, glm::vec3(0.85f, 0.80f, 0.70f), 0.80f },  // F ring, thin
	});
	// Uranus, radius 25,362 km: narrow, dark, carbon-rich rings.
	buildRingSystem("uranus", {
		{ 1.648f, 1.662f, glm::vec3(0.36f, 0.37f, 0.39f), 0.80f },  // 6/5/4 ring group
		{ 1.750f, 1.760f, glm::vec3(0.38f, 0.39f, 0.41f), 0.80f },  // alpha ring
		{ 1.786f, 1.796f, glm::vec3(0.38f, 0.39f, 0.41f), 0.80f },  // beta ring
		{ 1.860f, 1.876f, glm::vec3(0.40f, 0.41f, 0.43f), 0.80f },  // gamma/eta/delta group
		{ 2.000f, 2.020f, glm::vec3(0.50f, 0.51f, 0.54f), 0.90f },  // epsilon ring, widest
	});
	// Jupiter, radius 69,911 km: tenuous dust â€” halo and main ring, faint.
	buildRingSystem("jupiter", {
		{ 1.40f, 1.71f, glm::vec3(0.55f, 0.47f, 0.40f), 0.12f },    // halo ring
		{ 1.72f, 1.81f, glm::vec3(0.66f, 0.56f, 0.46f), 0.28f },    // main ring
	});
	// Neptune, radius 24,622 km: Galle, Le Verrier and Adams, faint.
	buildRingSystem("neptune", {
		{ 1.69f, 1.73f, glm::vec3(0.45f, 0.45f, 0.48f), 0.18f },    // Galle
		{ 2.14f, 2.16f, glm::vec3(0.55f, 0.55f, 0.58f), 0.30f },    // Le Verrier
		{ 2.53f, 2.55f, glm::vec3(0.58f, 0.58f, 0.62f), 0.35f },    // Adams
	});
	std::cout << "[SCENE] ring systems attached: Saturn (5 bands, Cassini Division gap), "
				 "Uranus (5), Jupiter (2 faint), Neptune (3 faint)" << std::endl;
}

void Application::buildEphemeris()
{
	m_ephemeris.setSunPosition(m_sunPosition);
	for (CelestialBody* body : m_solarSystem.bodies())
	{
		const BodyType type = body->data().type;
		if (type != BodyType::Planet && type != BodyType::DwarfPlanet)
			continue;
		m_ephemeris.addPlanet(body->data().id, body->data().displayName, body->data().radiusKm,
			body->transform().scale.x);
	}
	for (CelestialBody* body : m_solarSystem.bodies())
	{
		if (const MissionEphemeris::Planet* planet = m_ephemeris.findPlanet(body->data().id))
			m_planetBindings.push_back({ body, planet });
	}

	if (m_ephemeris.loadVoyager("assets/trajectory/voyager2_heliocentric.csv"))
	{
		m_ephemeris.computeEncounters({ "jupiter", "saturn", "uranus", "neptune" });
		const Trajectory& track = m_ephemeris.voyagerTrack();
		m_clock.setRange(track.startJulianDate(), track.endJulianDate());
		std::vector<double> encounterDates;
		for (const MissionEphemeris::Encounter& encounter : m_ephemeris.encounters())
			encounterDates.push_back(encounter.closestApproachJulianDate);
		m_clock.setEncounters(std::move(encounterDates));
		m_clock.setJulianDate(track.startJulianDate());
	}
	else
	{
		// Without the Voyager table the planets still run on their own data.
		m_clock.setRange(2443376.5, 2462503.5);
		m_clock.setJulianDate(2443376.5);
	}

	std::cout << "[TRAJECTORY] synchronized ephemeris: " << m_planetBindings.size()
			  << " planets + Voyager 2 on one simulation date" << std::endl;
}

void Application::buildVoyager()
{
	VoyagerModelBuildResult model = VoyagerModelBuilder::build();
	auto voyager = std::move(model.spacecraft);
	m_voyager = voyager.get();
	// 13 m magnetometer boom tip to RTG boom tip, with margin.
	m_voyager->setBoundingRadius(ScaleManager().spacecraftSizeToRenderUnits(9.0));

	if (m_ephemeris.hasVoyager())
	{
		// The drawn path is the SAME function the probe follows: every Horizons
		// row (10-minute spacing at each flyby) through voyagerRenderPosition.
		std::vector<glm::dvec3> path;
		path.reserve(m_ephemeris.voyagerTrack().samples().size());
		for (const TrajectorySample& sample : m_ephemeris.voyagerTrack().samples())
			path.push_back(m_ephemeris.voyagerRenderPosition(sample.julianDate));

		auto pathObject = std::make_unique<SceneObject>("voyager2_historical_trajectory");
		pathObject->setMesh(polylineMesh(path, m_sunPosition, PrimitiveMode::LineStrip));
		pathObject->setMaterial(flatMaterial(glm::vec3(0.95f, 0.72f, 0.30f)));
		pathObject->transform().position = m_sunPosition;
		pathObject->setVisible(m_trajectoryVisible);
		m_voyagerTrajectory = pathObject.get();
		m_scene.addObject(std::move(pathObject));
	}

	m_scene.addObject(std::move(voyager));

	std::cout << "[VOYAGER] procedural spacecraft built (" << model.visiblePartCount
			  << " visible assemblies, " << model.renderedTriangleCount
			  << " rendered triangles), Historical mode" << std::endl;
}

void Application::buildEnvironment()
{
	const ScaleManager scaleManager;

	// --- Background starfield: three brightness layers, one draw each,
	// always centred on the camera and never occluding the scene. ---
	struct StarLayer { unsigned int count; unsigned int seed; glm::vec3 color; };
	const std::array<StarLayer, 3> starLayers = {{
		{ 5200, 11u, glm::vec3(0.34f, 0.35f, 0.40f) },
		{ 1800, 23u, glm::vec3(0.66f, 0.66f, 0.72f) },
		{ 380, 37u, glm::vec3(1.0f, 0.97f, 0.92f) },
	}};
	for (const StarLayer& layer : starLayers)
	{
		auto stars = std::make_unique<SceneObject>("starfield");
		stars->setMesh(std::make_shared<Mesh>(StarfieldGenerator::generate(layer.count, 5000.0f, layer.seed),
			PrimitiveMode::Points));
		stars->setMaterial(flatMaterial(layer.color));
		m_backgroundLayers.push_back(std::move(stars));
	}

	// --- Orbit guides: the osculating two-body ellipse through each planet's
	// Horizons state, mapped with the same distance law, so every planet rides
	// its own line (3D: inclination and perihelion direction included). ---
	for (const PlanetBinding& binding : m_planetBindings)
	{
		const std::vector<glm::dvec3> guide = m_ephemeris.orbitGuide(*binding.ephemeris,
			kOrbitGuideEpochJulianDate, 360);
		if (guide.empty())
			continue;
		const bool dwarf = binding.body->data().type == BodyType::DwarfPlanet;
		auto orbit = std::make_unique<SceneObject>(binding.body->data().id + "_orbit");
		orbit->setMesh(polylineMesh(guide, m_sunPosition, PrimitiveMode::LineLoop));
		orbit->setMaterial(flatMaterial(dwarf ? glm::vec3(0.24f, 0.22f, 0.30f) : glm::vec3(0.22f, 0.30f, 0.40f)));
		orbit->transform().position = m_sunPosition;
		m_orbitGuides.push_back(orbit.get());
		m_scene.addObject(std::move(orbit));
	}

	// --- Heliosphere markers: three great circles per boundary. Voyager 2
	// crossed the termination shock at 84 AU (2007) and the heliopause at
	// 119 AU (2018-11-05). ---
	auto circleMesh = std::make_shared<Mesh>(CircleGenerator::generate(), PrimitiveMode::LineLoop);
	auto addSphericalBoundary = [this, &circleMesh](const std::string& name, double radius, const glm::vec3& color)
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
			loop->transform().position = m_sunPosition;
			loop->transform().rotation = rotations[plane];
			loop->transform().scale = glm::dvec3(radius);
			m_scene.addObject(std::move(loop));
		}
	};
	addSphericalBoundary("termination_shock", scaleManager.distanceAuToRenderUnits(84.0),
		glm::vec3(0.09f, 0.17f, 0.24f));
	addSphericalBoundary("heliopause", scaleManager.distanceAuToRenderUnits(119.0),
		glm::vec3(0.18f, 0.13f, 0.28f));

	// --- Small-body fields: one instanced draw call each (bible F9). ---
	const MeshData rockData = UvSphereGenerator::generate(6, 8);
	auto addInstancedField = [this, &rockData](const std::string& name, std::vector<glm::mat4> matrices,
		const glm::vec3& color)
	{
		auto mesh = std::make_shared<Mesh>(rockData);
		mesh->setInstanceTransforms(matrices);
		auto field = std::make_unique<InstancedField>(name);
		field->setMesh(mesh);
		field->setMaterial(flatMaterial(color, ShadingModel::Lit));
		const std::size_t count = matrices.size();
		m_scene.addObject(std::move(field));
		std::cout << "[SCENE] " << name << ": " << count << " instances, 1 draw call" << std::endl;
	};

	auto radiusAt = [&scaleManager](double au)
	{
		return static_cast<float>(scaleManager.distanceAuToRenderUnits(au));
	};
	addInstancedField("asteroid_belt",
		generateBeltMatrices(4000, radiusAt(2.1), radiusAt(3.3), 0.6f, 0.012f, 0.05f, m_sunPosition, 101),
		glm::vec3(0.62f, 0.57f, 0.50f));
	addInstancedField("kuiper_belt",
		generateBeltMatrices(3000, radiusAt(30.0), radiusAt(50.0), 3.0f, 0.06f, 0.20f, m_sunPosition, 202),
		glm::vec3(0.62f, 0.66f, 0.74f));
	addInstancedField("oort_cloud",
		generateShellMatrices(1500, radiusAt(2000.0), radiusAt(5000.0), 1.5f, 4.0f, m_sunPosition, 303),
		glm::vec3(0.70f, 0.76f, 0.84f));

	// --- Drifting comet: nucleus plus a tail that always points away from
	// the Sun (updateComet). The group is an orbiting, mesh-less CelestialBody. ---
	CelestialBodyData cometData;
	cometData.id = "drifting_comet";
	cometData.displayName = "Drifting Comet";
	auto cometGroup = std::make_unique<CelestialBody>(cometData, nullptr, nullptr);
	m_driftingComet = cometGroup.get();
	cometGroup->transform().scale = glm::dvec3(1.0);
	cometGroup->setOrbit(scaleManager.distanceAuToRenderUnits(60.0), 0.008, 4.712389, m_sunPosition, 0.35);

	auto cometNucleus = std::make_unique<SceneObject>("drifting_comet_nucleus");
	cometNucleus->setMesh(m_sphereMesh);
	cometNucleus->setMaterial(flatMaterial(glm::vec3(0.82f, 0.92f, 1.0f)));
	cometNucleus->transform().scale = glm::dvec3(0.25);
	cometGroup->addChild(std::move(cometNucleus));

	auto cometComa = std::make_unique<SceneObject>("drifting_comet_coma");
	cometComa->setMesh(m_sphereMesh);
	cometComa->setMaterial(glowMaterial(glm::vec3(0.55f, 0.85f, 1.0f), 0.8f, 2.0f));
	cometComa->transform().scale = glm::dvec3(0.9);
	cometGroup->addChild(std::move(cometComa));

	constexpr float kTailLength = 9.0f;
	auto cometTail = std::make_unique<SceneObject>("drifting_comet_tail");
	cometTail->setMesh(std::make_shared<Mesh>(CylinderGenerator::generate(0.35f, 0.0f, kTailLength, 16)));
	cometTail->setMaterial(flatMaterial(glm::vec3(0.62f, 0.88f, 1.0f), ShadingModel::Unlit, 0.45f));
	m_driftingCometTail = cometTail.get();
	cometGroup->addChild(std::move(cometTail));
	m_scene.addObject(std::move(cometGroup));

	std::cout << "[SCENE] environment built: 7,380-star background, " << m_orbitGuides.size()
			  << " ephemeris orbit guides, termination-shock/heliopause wireframes, 3 small-body fields, "
				 "1 drifting comet" << std::endl;
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
			if (saveScreenshot(path.str()))
				std::cout << "[APP] screenshot saved: " << path.str() << std::endl;
		}

		if (!m_captureShots.empty() && m_captureIndex < m_captureShots.size() &&
			(m_captureSecondsRemaining -= m_time.deltaTime()) <= 0.0)
		{
			const std::string path = m_captureDirectory + "/" + m_captureShots[m_captureIndex].fileName;
			if (saveScreenshot(path))
				std::cout << "[APP] capture saved: " << path << std::endl;
			if (++m_captureIndex < m_captureShots.size())
			{
				m_captureShots[m_captureIndex].setup(*this);
				m_captureSecondsRemaining = m_captureShots[m_captureIndex].settleSeconds;
			}
			else
			{
				m_window.requestClose();
			}
		}

		m_window.swapBuffers();
		m_window.pollEvents();
	}

	std::cout << "[APP] shutting down after " << m_time.frameCount() << " frames" << std::endl;
}

void Application::handleGlobalKeys()
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
		if (m_cameraMode == CameraMode::FreeFly)
			enterChase();
		else
			m_cameraMode = CameraMode::FreeFly;
		std::cout << "[APP] camera mode: " << (m_cameraMode == CameraMode::FreeFly ? "FreeFly" : "Chase") << std::endl;
	}
	if (m_input.keyPressed(GLFW_KEY_HOME) || m_input.keyPressed(GLFW_KEY_H))
		goToOverview();

	if (m_input.keyPressed(GLFW_KEY_TAB) && !m_solarSystem.bodies().empty())
	{
		const int count = static_cast<int>(m_solarSystem.bodies().size());
		const bool backward = m_input.keyDown(GLFW_KEY_LEFT_SHIFT) || m_input.keyDown(GLFW_KEY_RIGHT_SHIFT);
		focusBody(((m_focusIndex + (backward ? -1 : 1)) % count + count) % count);
	}
	if (m_input.keyPressed(GLFW_KEY_G) && m_focusIndex >= 0)
		focusBody(m_focusIndex);

	if (m_input.keyPressed(GLFW_KEY_M))
		m_mouseLookLatched = !m_mouseLookLatched;
	if (m_input.keyPressed(GLFW_KEY_L))
		m_labelsVisible = !m_labelsVisible;
	if (m_input.keyPressed(GLFW_KEY_K))
	{
		m_renderer.setLightingEnabled(!m_renderer.lightingEnabled());
		std::cout << "[APP] Sun lighting " << (m_renderer.lightingEnabled() ? "on" : "off") << std::endl;
	}
	if (m_input.keyPressed(GLFW_KEY_O))
	{
		m_orbitGuidesVisible = !m_orbitGuidesVisible;
		for (SceneObject* guide : m_orbitGuides)
			guide->setVisible(m_orbitGuidesVisible);
	}
	if (m_input.keyPressed(GLFW_KEY_N))
	{
		m_clock.setEncounterSlowdown(!m_clock.encounterSlowdown());
		std::cout << "[APP] encounter slow-motion " << (m_clock.encounterSlowdown() ? "on" : "off") << std::endl;
	}

	if (m_input.keyPressed(GLFW_KEY_P))
	{
		m_clock.setPaused(!m_clock.paused());
		std::cout << "[APP] simulation " << (m_clock.paused() ? "paused" : "resumed") << std::endl;
	}
	if (m_input.keyPressed(GLFW_KEY_EQUAL) || m_input.keyPressed(GLFW_KEY_RIGHT_BRACKET) ||
		m_input.keyPressed(GLFW_KEY_KP_ADD))
	{
		m_simulationSpeed = std::min(m_simulationSpeed * 2.0, 64.0);
		std::cout << "[APP] simulation speed: " << m_simulationSpeed << "x" << std::endl;
	}
	if (m_input.keyPressed(GLFW_KEY_MINUS) || m_input.keyPressed(GLFW_KEY_LEFT_BRACKET) ||
		m_input.keyPressed(GLFW_KEY_KP_SUBTRACT))
	{
		m_simulationSpeed = std::max(m_simulationSpeed / 2.0, 1.0 / 64.0);
		std::cout << "[APP] simulation speed: " << m_simulationSpeed << "x" << std::endl;
	}
	if (m_input.keyPressed(GLFW_KEY_BACKSPACE))
	{
		m_simulationSpeed = 1.0;
		m_clock.setPaused(false);
	}

	if (m_voyager != nullptr && m_input.keyPressed(GLFW_KEY_V))
	{
		const bool wasManual = m_voyager->flightMode() == Voyager2::FlightMode::Manual;
		m_voyager->setFlightMode(wasManual ? Voyager2::FlightMode::Historical : Voyager2::FlightMode::Manual);
		if (!wasManual && m_cameraMode != CameraMode::Chase)
			enterChase();
		if (wasManual)
			updateVoyagerHistorical(true);
		std::cout << "[VOYAGER] flight mode: " << (wasManual ? "Historical" : "Manual") << std::endl;
	}
	if (m_voyagerTrajectory != nullptr && m_input.keyPressed(GLFW_KEY_T))
	{
		m_trajectoryVisible = !m_trajectoryVisible;
		m_voyagerTrajectory->setVisible(m_trajectoryVisible);
		std::cout << "[TRAJECTORY] line " << (m_trajectoryVisible ? "shown" : "hidden") << std::endl;
	}

	const std::array<int, 6> bookmarkKeys = { GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4, GLFW_KEY_5, GLFW_KEY_6 };
	for (std::size_t i = 0; i < bookmarkKeys.size(); ++i)
	{
		if (m_input.keyPressed(bookmarkKeys[i]))
			jumpToBookmark(static_cast<int>(i));
	}
}

void Application::update(double deltaTime)
{
	handleGlobalKeys();

	m_clock.setSpeed(m_simulationSpeed);
	m_clock.update(deltaTime);
	CelestialBody::setSimulationTimeScale(m_clock.paused() ? 0.0 : m_simulationSpeed);

	// Manual piloting only when the chase camera owns the keyboard; in free
	// flight the same keys move the camera instead.
	if (m_voyager != nullptr && m_voyager->flightMode() == Voyager2::FlightMode::Manual &&
		m_cameraMode == CameraMode::Chase)
		m_voyager->applyManualControl(m_input, deltaTime);

	m_scene.update(deltaTime);
	updateEphemerisPositions();
	if (m_voyager != nullptr && m_voyager->flightMode() == Voyager2::FlightMode::Historical)
		updateVoyagerHistorical(false);
	updateComet();
	updateCamera(deltaTime);

	m_titleRefreshTimer += deltaTime;
	if (m_titleRefreshTimer >= 0.5)
	{
		refreshWindowTitle();
		m_titleRefreshTimer = 0.0;
	}
}

void Application::updateEphemerisPositions()
{
	const double julianDate = m_clock.julianDate();
	for (const PlanetBinding& binding : m_planetBindings)
		binding.body->transform().position = m_ephemeris.planetRenderPosition(*binding.ephemeris, julianDate);
}

void Application::updateVoyagerHistorical(bool snap)
{
	if (m_voyager == nullptr || !m_ephemeris.hasVoyager())
		return;

	const double julianDate = m_clock.julianDate();
	// Heading from a short central difference on the rendered path; the step
	// shrinks with the encounter factor so the flyby turn is resolved.
	const double step = std::clamp(0.5 * m_clock.encounterFactor(), 0.002, 0.5);
	const glm::dvec3 before = m_ephemeris.voyagerRenderPosition(julianDate - step);
	const glm::dvec3 after = m_ephemeris.voyagerRenderPosition(julianDate + step);
	const double renderSpeed = glm::length(after - before) / (2.0 * step) * m_clock.daysPerSecond();
	m_voyager->setHistoricalState(m_ephemeris.voyagerRenderPosition(julianDate), after - before,
		renderSpeed, snap);
}

void Application::updateComet()
{
	if (m_driftingComet == nullptr || m_driftingCometTail == nullptr)
		return;

	// Real comet tails point away from the Sun: rotate the tail's +Y axis
	// onto the anti-Sun direction every frame.
	const glm::dvec3 awayFromSun = glm::normalize(m_driftingComet->transform().position - m_sunPosition);
	const glm::dvec3 yAxis(0.0, 1.0, 0.0);
	const glm::dvec3 axis = glm::cross(yAxis, awayFromSun);
	const double axisLength = glm::length(axis);
	glm::dquat rotation(1.0, 0.0, 0.0, 0.0);
	if (axisLength < 1e-9)
	{
		if (glm::dot(yAxis, awayFromSun) < 0.0)
			rotation = glm::angleAxis(glm::pi<double>(), glm::dvec3(1.0, 0.0, 0.0));
	}
	else
	{
		rotation = glm::angleAxis(std::acos(glm::clamp(glm::dot(yAxis, awayFromSun), -1.0, 1.0)), axis / axisLength);
	}

	constexpr double kTailHalfLength = 4.5;
	m_driftingCometTail->transform().rotation = rotation;
	m_driftingCometTail->transform().position = rotation * glm::dvec3(0.0, kTailHalfLength, 0.0);
}

double Application::nearestSurfaceDistance(const glm::dvec3& point) const
{
	double nearest = glm::length(point - m_sunPosition);
	for (const CelestialBody* body : m_solarSystem.bodies())
	{
		const glm::dmat4 world = body->worldMatrix();
		const double radius = glm::length(glm::dvec3(world[0]));
		nearest = std::min(nearest, glm::length(point - glm::dvec3(world[3])) - radius);
	}
	if (m_voyager != nullptr)
		nearest = std::min(nearest, glm::length(point - m_voyager->transform().position) - m_voyager->boundingRadius());
	return std::max(nearest, 0.0);
}

void Application::updateCamera(double deltaTime)
{
	const bool movementKey = m_input.keyDown(GLFW_KEY_W) || m_input.keyDown(GLFW_KEY_A) ||
		m_input.keyDown(GLFW_KEY_S) || m_input.keyDown(GLFW_KEY_D) || m_input.keyDown(GLFW_KEY_Q) ||
		m_input.keyDown(GLFW_KEY_E) || m_input.keyDown(GLFW_KEY_SPACE) || m_input.keyDown(GLFW_KEY_LEFT_CONTROL);
	const bool pilotingVoyager = m_voyager != nullptr &&
		m_voyager->flightMode() == Voyager2::FlightMode::Manual && m_cameraMode == CameraMode::Chase;

	// Full autonomy: any fly key hands a locked view straight to free flight
	// from exactly where the camera is (piloting Voyager excepted).
	if (movementKey && !pilotingVoyager && m_cameraMode != CameraMode::FreeFly && m_captureShots.empty())
		m_cameraMode = CameraMode::FreeFly;

	const bool look = m_mouseLookLatched || m_input.mouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT);
	m_input.setCursorCaptured(look);

	if (m_cameraMode == CameraMode::FreeFly)
	{
		// Speed follows the distance to the nearest surface: centimetres per
		// second beside Voyager, hundreds of units per second between planets.
		const double surface = nearestSurfaceDistance(m_camera.position());
		const double baseSpeed = std::clamp(surface * 0.9, 0.004, 300.0);
		m_camera.updateFreeFly(m_input, deltaTime, baseSpeed, look);
	}
	else if (m_cameraMode == CameraMode::Chase && m_voyager != nullptr)
	{
		const double radius = m_voyager->boundingRadius();
		m_camera.updateOrbit(m_voyager->transform().position, chaseFrame(),
			radius * 1.3, 5000.0, m_input, deltaTime, look);
	}
	else if (m_cameraMode == CameraMode::Focus && m_focusIndex >= 0 &&
			 m_focusIndex < static_cast<int>(m_solarSystem.bodies().size()))
	{
		const CelestialBody* focused = m_solarSystem.bodies()[m_focusIndex];
		const glm::dmat4 world = focused->worldMatrix();
		const double radius = glm::length(glm::dvec3(world[0]));
		m_camera.updateOrbit(glm::dvec3(world[3]), glm::dquat(1.0, 0.0, 0.0, 0.0), radius * 1.08,
			5000.0, m_input, deltaTime, look);
	}
}

glm::dquat Application::chaseFrame() const
{
	// Normally the rig rides in Voyager's own frame. During a historical
	// flyby it turns to face the planet instead, so the classic shot — the
	// spacecraft in the foreground, the world it is passing behind it — holds
	// through the whole encounter. The camera smooths the switch.
	if (m_voyager->flightMode() == Voyager2::FlightMode::Historical)
	{
		const glm::dvec3 voyagerPosition = m_voyager->transform().position;
		auto facePlanet = [&](const std::string& id, double range, glm::dquat& frame)
		{
			const CelestialBody* planet = m_solarSystem.find(id);
			if (planet == nullptr)
				return false;
			const glm::dvec3 toPlanet = planet->transform().position - voyagerPosition;
			const double distance = glm::length(toPlanet);
			if (distance >= range || distance <= 1e-9)
				return false;
			frame = frameLookingAlong(toPlanet / distance);
			return true;
		};
		glm::dquat frame;
		for (const MissionEphemeris::Encounter& encounter : m_ephemeris.encounters())
		{
			if (facePlanet(encounter.planetId, encounter.clearanceRenderUnits * 6.0, frame))
				return frame;
		}
		// Departure: look back at Earth while Voyager is still beside it.
		const CelestialBody* earth = m_solarSystem.find("earth");
		if (earth != nullptr && facePlanet("earth", earth->transform().scale.x * 12.0, frame))
			return frame;
	}
	return m_voyager->orientation();
}

glm::dquat Application::frameLookingAlong(const glm::dvec3& forward)
{
	glm::dvec3 upReference(0.0, 1.0, 0.0);
	if (std::abs(glm::dot(forward, upReference)) > 0.999)
		upReference = glm::dvec3(1.0, 0.0, 0.0);
	const glm::dvec3 right = glm::normalize(glm::cross(upReference, forward));
	const glm::dvec3 up = glm::cross(forward, right);
	return glm::normalize(glm::quat_cast(glm::dmat3(right, up, forward)));
}

void Application::goToOverview()
{
	m_cameraMode = CameraMode::FreeFly;
	m_camera.setPosition(m_sunPosition + glm::dvec3(0.0, 62.0, 150.0));
	m_camera.lookAt(m_sunPosition + glm::dvec3(0.0, 0.0, 12.0));
	std::cout << "[APP] overview camera" << std::endl;
}

void Application::focusBody(int index)
{
	if (index < 0 || index >= static_cast<int>(m_solarSystem.bodies().size()))
		return;
	m_focusIndex = index;
	m_cameraMode = CameraMode::Focus;
	const CelestialBody* body = m_solarSystem.bodies()[index];
	const double radius = glm::length(glm::dvec3(body->worldMatrix()[0]));
	// Planets with moons open wide enough to show their system.
	const bool hasMoons = std::any_of(body->children().begin(), body->children().end(),
		[](const std::unique_ptr<SceneObject>& child)
		{
			return dynamic_cast<const CelestialBody*>(child.get()) != nullptr;
		});
	const double distance = radius * (body->data().type == BodyType::Star ? 4.0 : (hasMoons ? 5.5 : 3.2));
	// Open on the day side: the camera sits 40 degrees round from the
	// body-to-Sun direction (local offset is (sin yaw, -, -cos yaw)).
	const glm::dvec3 toSun = m_sunPosition - glm::dvec3(body->worldMatrix()[3]);
	float yaw = 25.0f;
	if (glm::length(glm::dvec3(toSun.x, 0.0, toSun.z)) > 1e-6)
		yaw = static_cast<float>(glm::degrees(std::atan2(toSun.x, -toSun.z))) + 40.0f;
	m_camera.beginOrbit(distance, yaw, 18.0f);
	std::cout << "[APP] focused: " << body->data().displayName << std::endl;
}

void Application::enterChase()
{
	m_cameraMode = CameraMode::Chase;
	// A three-quarter view from behind and above; wheel zooms, RMB orbits.
	m_camera.beginOrbit(m_voyager != nullptr ? m_voyager->boundingRadius() * 4.0 : 0.1, 20.0f, 10.0f);
}

void Application::jumpToBookmark(int index)
{
	if (index < 0 || index >= static_cast<int>(kBookmarks.size()) || m_voyager == nullptr)
		return;

	const Bookmark& bookmark = kBookmarks[index];
	double julianDate = bookmark.julianDate;
	if (index == 0)
		julianDate = m_clock.startJulianDate();
	for (const MissionEphemeris::Encounter& encounter : m_ephemeris.encounters())
	{
		if (encounter.planetId == bookmark.planetId)
			julianDate = encounter.closestApproachJulianDate - kBookmarkLeadDays;
	}

	m_voyager->setFlightMode(Voyager2::FlightMode::Historical);
	m_clock.setJulianDate(julianDate);
	m_clock.setPaused(false);
	updateEphemerisPositions();
	updateVoyagerHistorical(true);
	enterChase();
	std::cout << "[TRAJECTORY] bookmark: " << bookmark.name << std::endl;
}

void Application::freezeBeforeEncounter(int bookmark, double daysBefore)
{
	jumpToBookmark(bookmark);
	for (const MissionEphemeris::Encounter& encounter : m_ephemeris.encounters())
	{
		if (encounter.planetId == kBookmarks[bookmark].planetId)
			m_clock.setJulianDate(encounter.closestApproachJulianDate - daysBefore);
	}
	m_clock.setPaused(true);
	updateEphemerisPositions();
	updateVoyagerHistorical(true);
}

void Application::refreshWindowTitle()
{
	std::ostringstream title;
	title << "Voyager 2 Explorer | " << formatJulianDate(m_clock.julianDate()) << " | F1: controls";
	m_window.setTitle(title.str());
}

void Application::render()
{
	const CelestialBody* sun = m_solarSystem.find("sun");
	m_renderer.setLight(sun != nullptr ? sun->transform().position : m_sunPosition, glm::vec3(1.0f, 0.98f, 0.94f));
	m_renderer.beginFrame(m_camera, m_window.aspectRatio());
	for (const auto& layer : m_backgroundLayers)
		m_renderer.submitBackground(*layer->mesh(), *layer->material());
	m_scene.render(m_renderer);
	m_renderer.endFrame();
	renderOverlay();
}

void Application::renderOverlay()
{
	m_text.begin(m_window.width(), m_window.height());
	if (m_labelsVisible)
		renderLabels();
	if (m_hudVisible)
		renderHud();
	m_text.flush();
}

void Application::renderLabels()
{
	const float width = static_cast<float>(m_window.width());
	const float height = static_cast<float>(m_window.height());
	const glm::mat4 viewProjection = m_camera.projectionMatrix(m_window.aspectRatio()) * m_camera.viewMatrixAtOrigin();
	const glm::dvec3 eye = m_camera.position();
	const float pixelSize = std::max(1.0f, std::round(height / 450.0f));

	// Returns false when the point is behind the camera or off-screen.
	auto project = [&](const glm::dvec3& world, glm::vec2& pixel) -> bool
	{
		const glm::vec4 clip = viewProjection * glm::vec4(glm::vec3(world - eye), 1.0f);
		if (clip.w <= 0.0f)
			return false;
		const glm::vec3 ndc = glm::vec3(clip) / clip.w;
		if (std::abs(ndc.x) > 1.1f || std::abs(ndc.y) > 1.1f)
			return false;
		pixel = glm::vec2((ndc.x * 0.5f + 0.5f) * width, (0.5f - ndc.y * 0.5f) * height);
		return true;
	};
	const float focalPixels = height * 0.5f / std::tan(glm::radians(m_camera.fieldOfView()) * 0.5f);
	// Greedy declutter: labels are placed in priority order (Sun, planets,
	// Voyager, then moons) and one that would overlap a placed label is skipped.
	std::vector<glm::vec4> placed;
	if (m_hudVisible)
		placed.push_back(m_hudPanelRect); // never write names under the HUD text

	// A name is hidden when any nearer world covers the point it labels.
	auto occluded = [&](const glm::dvec3& world, double radius)
	{
		const glm::dvec3 toTarget = world - eye;
		const double targetDistance = glm::length(toTarget);
		if (targetDistance <= 1e-12)
			return false;
		const glm::dvec3 direction = toTarget / targetDistance;
		for (const CelestialBody* blocker : m_solarSystem.bodies())
		{
			const glm::dmat4 blockerWorld = blocker->worldMatrix();
			const glm::dvec3 centre(blockerWorld[3]);
			const double blockerRadius = glm::length(glm::dvec3(blockerWorld[0]));
			if (glm::length(centre - world) < 1e-9)
				continue; // the labelled body itself
			const double along = glm::dot(centre - eye, direction);
			if (along <= 0.0 || along >= targetDistance - radius)
				continue;
			if (glm::length(eye + direction * along - centre) < blockerRadius)
				return true;
		}
		return false;
	};

	auto drawLabel = [&](const glm::dvec3& world, double radius, const std::string& text, const glm::vec4& color)
	{
		glm::vec2 pixel;
		if (!project(world, pixel) || occluded(world, radius))
			return;
		const double distance = glm::length(world - eye);
		const float projectedRadius = static_cast<float>(radius / std::max(distance, 1e-9)) * focalPixels;
		if (projectedRadius > height * 0.45f)
			return; // filling the screen: the name would only hide the surface
		const float labelWidth = TextRenderer::textWidth(text, pixelSize);
		const float y = pixel.y - projectedRadius - pixelSize * 11.0f;
		const glm::vec4 box(pixel.x - labelWidth * 0.5f - pixelSize, y - pixelSize,
			pixel.x + labelWidth * 0.5f + pixelSize, y + pixelSize * 8.0f);
		for (const glm::vec4& other : placed)
		{
			if (box.x < other.z && box.z > other.x && box.y < other.w && box.w > other.y)
				return;
		}
		placed.push_back(box);
		m_text.addShadowedText(pixel.x - labelWidth * 0.5f, y, text, pixelSize, color);
		// A small tick connects the name to a body too small to see.
		if (projectedRadius < pixelSize * 2.0f)
			m_text.addRect(pixel.x - pixelSize * 0.5f, y + pixelSize * 8.5f, pixelSize, pixelSize * 2.0f, color * 0.8f);
	};

	auto labelBody = [&](const CelestialBody* body)
	{
		const glm::dmat4 world = body->worldMatrix();
		const glm::dvec3 position(world[3]);
		const double radius = glm::length(glm::dvec3(world[0]));
		const BodyType type = body->data().type;
		if (type != BodyType::Moon)
		{
			drawLabel(position, radius, body->data().displayName,
				type == BodyType::Star ? glm::vec4(1.0f, 0.86f, 0.45f, 1.0f) : glm::vec4(0.95f, 0.95f, 0.95f, 1.0f));
			return;
		}

		// Moons are named only when the camera is inside their system.
		const CelestialBody* parent = m_solarSystem.find(body->data().parentId);
		if (parent == nullptr)
			return;
		const glm::dmat4 parentWorld = parent->worldMatrix();
		const double parentRadius = glm::length(glm::dvec3(parentWorld[0]));
		if (glm::length(glm::dvec3(parentWorld[3]) - eye) <= parentRadius * 10.0)
			drawLabel(position, radius, body->data().displayName, glm::vec4(0.62f, 0.80f, 1.0f, 0.95f));
	};

	// Nearer worlds win overlaps, so the planet being visited is never
	// hidden behind the name of a distant one on the same line of sight.
	std::vector<const CelestialBody*> majorBodies;
	for (const CelestialBody* body : m_solarSystem.bodies())
	{
		if (body->data().type != BodyType::Moon)
			majorBodies.push_back(body);
	}
	std::sort(majorBodies.begin(), majorBodies.end(), [&eye](const CelestialBody* a, const CelestialBody* b)
	{
		return glm::length(a->transform().position - eye) < glm::length(b->transform().position - eye);
	});
	for (const CelestialBody* body : majorBodies)
		labelBody(body);
	if (m_voyager != nullptr && m_cameraMode != CameraMode::Chase)
		drawLabel(m_voyager->transform().position, m_voyager->boundingRadius(), "Voyager 2",
			glm::vec4(1.0f, 0.78f, 0.30f, 1.0f));
	for (const CelestialBody* body : m_solarSystem.bodies())
	{
		if (body->data().type == BodyType::Moon)
			labelBody(body);
	}
}

void Application::renderHud()
{
	const float height = static_cast<float>(m_window.height());
	const float width = static_cast<float>(m_window.width());
	const float pixel = std::max(1.0f, std::round(height / 450.0f));
	const float line = TextRenderer::lineHeight(pixel);
	const glm::vec4 white(0.92f, 0.94f, 0.97f, 1.0f);
	const glm::vec4 accent(1.0f, 0.78f, 0.30f, 1.0f);
	const glm::vec4 dim(0.62f, 0.68f, 0.76f, 1.0f);

	std::vector<std::pair<std::string, glm::vec4>> lines;
	const double julianDate = m_clock.julianDate();
	lines.push_back({ "VOYAGER 2  SOLAR SYSTEM EXPLORER", accent });
	lines.push_back({ formatJulianDate(julianDate) + "   JD " + fixed(julianDate, 2), white });

	std::string timeLine;
	if (m_clock.paused())
		timeLine = "TIME PAUSED (P)";
	else
	{
		const double daysPerSecond = m_clock.daysPerSecond();
		timeLine = daysPerSecond >= 1.0 ? "TIME " + fixed(daysPerSecond, 1) + " DAYS/S"
			: "TIME " + fixed(daysPerSecond * 24.0, 2) + " HOURS/S";
		timeLine += "  SPEED " + fixed(m_simulationSpeed, m_simulationSpeed < 1.0 ? 3 : 0) + "X";
		if (m_clock.encounterFactor() < 0.999)
			timeLine += "  ENCOUNTER SLOW-MOTION";
	}
	lines.push_back({ timeLine, white });

	const char* cameraName = m_cameraMode == CameraMode::FreeFly ? "FREE FLIGHT"
		: (m_cameraMode == CameraMode::Chase ? "CHASE VOYAGER" : "FOCUS");
	std::string cameraLine = std::string("CAMERA ") + cameraName;
	if (m_cameraMode == CameraMode::FreeFly)
		cameraLine += "  (WHEEL SPEED X" + fixed(m_camera.speedMultiplier(), 2) + ")";
	if (m_cameraMode == CameraMode::Focus && m_focusIndex >= 0)
		cameraLine += ": " + m_solarSystem.bodies()[m_focusIndex]->data().displayName;
	lines.push_back({ cameraLine, white });

	if (m_voyager != nullptr)
	{
		const bool historical = m_voyager->flightMode() == Voyager2::FlightMode::Historical;
		lines.push_back({ std::string("FLIGHT ") + (historical ? "HISTORICAL (NASA/JPL HORIZONS)" : "MANUAL PILOT"),
			historical ? white : accent });
		if (historical && m_ephemeris.hasVoyager())
		{
			const MissionEphemeris::Telemetry telemetry = m_ephemeris.voyagerTelemetry(julianDate);
			lines.push_back({ "VOYAGER " + fixed(telemetry.sunDistanceAu, 2) + " AU FROM SUN   " +
				fixed(telemetry.heliocentricSpeedKmPerSecond, 1) + " KM/S", white });
			lines.push_back({ "NEAREST " + telemetry.nearestPlanet + " " +
				formatThousands(telemetry.nearestPlanetDistanceKm) + " KM", white });
		}
		else if (!historical)
		{
			lines.push_back({ "SPEED " + fixed(glm::length(m_voyager->velocity()), 3) + " UNITS/S  (X BRAKES)", white });
		}
	}
	if (m_cameraMode == CameraMode::Focus && m_focusIndex >= 0)
	{
		const CelestialBodyData& data = m_solarSystem.bodies()[m_focusIndex]->data();
		lines.push_back({ data.displayName + ": RADIUS " + formatThousands(data.radiusKm) + " KM", dim });
	}
	lines.push_back({ "F1 CONTROLS", dim });

	float panelWidth = 0.0f;
	for (const auto& entry : lines)
		panelWidth = std::max(panelWidth, TextRenderer::textWidth(entry.first, pixel));
	const float margin = pixel * 6.0f;
	m_hudPanelRect = glm::vec4(margin - pixel * 4.0f, margin - pixel * 4.0f, margin + panelWidth + pixel * 4.0f,
		margin + line * lines.size() + pixel * 1.0f);
	m_text.addRect(m_hudPanelRect.x, m_hudPanelRect.y, m_hudPanelRect.z - m_hudPanelRect.x,
		m_hudPanelRect.w - m_hudPanelRect.y, glm::vec4(0.0f, 0.02f, 0.05f, 0.55f));
	for (std::size_t i = 0; i < lines.size(); ++i)
		m_text.addText(margin, margin + line * static_cast<float>(i), lines[i].first, pixel, lines[i].second);

	// Encounter banner: counts down to the real closest approach.
	for (const MissionEphemeris::Encounter& encounter : m_ephemeris.encounters())
	{
		const double hours = (julianDate - encounter.closestApproachJulianDate) * 24.0;
		if (std::abs(hours) > 72.0)
			continue;
		const int totalMinutes = static_cast<int>(std::abs(hours) * 60.0);
		std::ostringstream clock;
		clock << (hours < 0.0 ? "T-" : "T+") << std::setw(2) << std::setfill('0') << totalMinutes / 60
			<< ':' << std::setw(2) << totalMinutes % 60;
		std::string upper = encounter.displayName;
		std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
		const std::string title = upper + " ENCOUNTER  " + clock.str();
		const std::string detail = "CLOSEST APPROACH " + formatThousands(encounter.closestApproachKm) + " KM FROM CENTRE";
		const float big = pixel * 1.5f;
		const float titleWidth = TextRenderer::textWidth(title, big);
		const float detailWidth = TextRenderer::textWidth(detail, pixel);
		const float bannerY = height - margin - TextRenderer::lineHeight(big) - line;
		m_text.addShadowedText((width - titleWidth) * 0.5f, bannerY, title, big, accent);
		m_text.addShadowedText((width - detailWidth) * 0.5f, bannerY + TextRenderer::lineHeight(big), detail, pixel, white);
	}

	if (m_helpVisible)
	{
		const std::vector<std::string> help = {
			"CONTROLS",
			"",
			"FREE CAMERA (C TOGGLES FREE / CHASE)",
			"  W A S D           FLY   (ANY FLY KEY LEAVES A LOCKED VIEW)",
			"  SPACE/E  CTRL/Q   UP / DOWN",
			"  MOUSE + RMB, OR M LOOK LOCK, OR ARROWS   LOOK",
			"  WHEEL             CRUISE SPEED (ZOOM IN LOCKED VIEWS)",
			"  SHIFT / ALT       FAST / FINE",
			"  TAB / SHIFT+TAB   FLY TO NEXT / PREVIOUS BODY",
			"  G                 RETURN TO SELECTED BODY",
			"  H / HOME          WHOLE SOLAR SYSTEM OVERVIEW",
			"",
			"MISSION",
			"  1 LAUNCH  2 JUPITER  3 SATURN  4 URANUS  5 NEPTUNE  6 HELIOPAUSE",
			"  P PAUSE   = / - SPEED   BACKSPACE RESET SPEED",
			"  N ENCOUNTER SLOW-MOTION   T VOYAGER PATH   O ORBIT GUIDES",
			"",
			"VOYAGER (V: HISTORICAL / MANUAL, MANUAL NEEDS CHASE CAMERA)",
			"  W/S THRUST   A/D YAW   R/F PITCH   Q/E ROLL",
			"  SPACE/CTRL UP/DOWN   SHIFT BOOST   X BRAKE",
			"",
			"DISPLAY",
			"  L LABELS   K SUN LIGHTING   F2 HUD   F12 SCREENSHOT   ESC QUIT",
		};
		float helpWidth = 0.0f;
		for (const std::string& entry : help)
			helpWidth = std::max(helpWidth, TextRenderer::textWidth(entry, pixel));
		const float helpHeight = line * help.size();
		const float x = (width - helpWidth) * 0.5f;
		const float y = (height - helpHeight) * 0.5f;
		m_text.addRect(x - pixel * 8.0f, y - pixel * 8.0f, helpWidth + pixel * 16.0f, helpHeight + pixel * 12.0f,
			glm::vec4(0.0f, 0.02f, 0.06f, 0.82f));
		for (std::size_t i = 0; i < help.size(); ++i)
			m_text.addText(x, y + line * static_cast<float>(i), help[i], pixel, i == 0 ? accent : white);
	}
}

bool Application::saveScreenshot(const std::string& path) const
{
	const int width = m_window.width();
	const int height = m_window.height();
	if (width <= 0 || height <= 0)
		return false;

	const int rowBytes = (width * 3 + 3) & ~3;
	std::vector<std::uint8_t> pixels(static_cast<std::size_t>(rowBytes) * height);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, pixels.data());

	std::ofstream file(path, std::ios::binary);
	if (!file)
		return false;

	// Uncompressed 24-bit BMP; OpenGL's bottom-up rows are BMP's native order.
	auto write32 = [&file](std::uint32_t value) { file.write(reinterpret_cast<const char*>(&value), 4); };
	auto write16 = [&file](std::uint16_t value) { file.write(reinterpret_cast<const char*>(&value), 2); };
	const std::uint32_t imageSize = static_cast<std::uint32_t>(pixels.size());
	file.put('B');
	file.put('M');
	write32(54 + imageSize);
	write32(0);
	write32(54);
	write32(40);
	write32(static_cast<std::uint32_t>(width));
	write32(static_cast<std::uint32_t>(height));
	write16(1);
	write16(24);
	write32(0);
	write32(imageSize);
	write32(2835);
	write32(2835);
	write32(0);
	write32(0);
	file.write(reinterpret_cast<const char*>(pixels.data()), static_cast<std::streamsize>(pixels.size()));
	return static_cast<bool>(file);
}

void Application::setupCaptureTour(const std::string& directory, bool everyBody)
{
	m_captureDirectory = directory;
	std::filesystem::create_directories(directory);

	if (everyBody)
	{
		// One Focus view per registered body, for the per-object documents.
		m_captureShots.clear();
		m_captureCursor = 0;
		for (const CelestialBody* body : m_solarSystem.bodies())
		{
			m_captureShots.push_back({ body->data().id + ".bmp", 2.6, [](Application& app)
			{
				app.m_clock.setPaused(true);
				app.focusBody(app.m_captureCursor++);
			} });
		}
		m_captureIndex = 0;
		m_captureShots[0].setup(*this);
		m_captureSecondsRemaining = m_captureShots[0].settleSeconds;
		std::cout << "[APP] body capture tour: " << m_captureShots.size() << " shots -> " << directory << std::endl;
		return;
	}

	auto focusById = [](Application& app, const char* id)
	{
		const auto& bodies = app.m_solarSystem.bodies();
		for (int i = 0; i < static_cast<int>(bodies.size()); ++i)
		{
			if (bodies[i]->data().id == id)
				app.focusBody(i);
		}
	};

	m_captureShots = {
		{ "01_overview.bmp", 1.4, [](Application& app) { app.m_clock.setPaused(true); app.goToOverview(); } },
		{ "02_launch_chase.bmp", 2.8, [](Application& app) { app.jumpToBookmark(0); app.m_clock.setPaused(true); } },
		{ "03_jupiter_approach.bmp", 3.0, [](Application& app) { app.freezeBeforeEncounter(1, 0.25); } },
		{ "04_saturn_approach.bmp", 3.0, [](Application& app) { app.freezeBeforeEncounter(2, 0.12); } },
		{ "05_uranus_approach.bmp", 3.0, [](Application& app) { app.freezeBeforeEncounter(3, 0.12); } },
		{ "06_neptune_approach.bmp", 3.0, [](Application& app) { app.freezeBeforeEncounter(4, 0.06); } },
		{ "07_heliopause.bmp", 2.8, [](Application& app) { app.jumpToBookmark(5); app.m_clock.setPaused(true); } },
		{ "08_focus_earth.bmp", 3.5, [](Application& app) { app.m_clock.setPaused(true); } },
		{ "09_focus_jupiter.bmp", 3.5, [](Application& app) { app.m_clock.setPaused(true); } },
		{ "10_focus_saturn.bmp", 3.5, [](Application& app) { app.m_clock.setPaused(true); } },
		{ "11_focus_uranus.bmp", 3.5, [](Application& app) { app.m_clock.setPaused(true); } },
		{ "12_help.bmp", 0.5, [](Application& app) { app.m_helpVisible = true; } },
	};
	// Captureless lambdas cannot capture focusById; the focus shots use a
	// small trampoline through a static pointer to it instead.
	static decltype(focusById) s_focus = focusById;
	m_captureShots[7].setup = [](Application& app) { app.jumpToBookmark(4); app.m_clock.setPaused(true); s_focus(app, "earth"); };
	m_captureShots[8].setup = [](Application& app) { app.m_clock.setPaused(true); s_focus(app, "jupiter"); };
	m_captureShots[9].setup = [](Application& app) { app.m_clock.setPaused(true); s_focus(app, "saturn"); };
	m_captureShots[10].setup = [](Application& app) { app.m_clock.setPaused(true); s_focus(app, "uranus"); };

	m_captureIndex = 0;
	m_captureShots[0].setup(*this);
	m_captureSecondsRemaining = m_captureShots[0].settleSeconds;
	std::cout << "[APP] capture tour: " << m_captureShots.size() << " shots -> " << directory << std::endl;
}
