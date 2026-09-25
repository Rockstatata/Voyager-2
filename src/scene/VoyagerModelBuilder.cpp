#include "VoyagerModelBuilder.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "ScaleManager.h"
#include "SceneObject.h"
#include "Voyager2.h"
#include "../rendering/BoxGenerator.h"
#include "../rendering/CylinderGenerator.h"
#include "../rendering/Material.h"
#include "../rendering/MaterialLibrary.h"
#include "../rendering/Mesh.h"
#include "../rendering/ParabolicDishGenerator.h"

namespace
{
	void appendTransformed(MeshData& destination, const MeshData& source, const glm::dmat4& transform)
	{
		const std::uint32_t firstVertex = static_cast<std::uint32_t>(destination.vertices.size());
		const glm::dmat3 normalMatrix = glm::transpose(glm::inverse(glm::dmat3(transform)));

		destination.vertices.reserve(destination.vertices.size() + source.vertices.size());
		for (const Vertex& sourceVertex : source.vertices)
		{
			Vertex vertex = sourceVertex;
			vertex.position = glm::vec3(transform * glm::dvec4(sourceVertex.position, 1.0));
			vertex.normal = glm::normalize(glm::vec3(normalMatrix * glm::dvec3(sourceVertex.normal)));
			destination.vertices.push_back(vertex);
		}

		destination.indices.reserve(destination.indices.size() + source.indices.size());
		for (std::uint32_t index : source.indices)
			destination.indices.push_back(firstVertex + index);
	}

	glm::dquat rotateYTo(const glm::dvec3& direction)
	{
		const glm::dvec3 yAxis(0.0, 1.0, 0.0);
		const glm::dvec3 target = glm::normalize(direction);
		const double cosine = glm::clamp(glm::dot(yAxis, target), -1.0, 1.0);
		if (cosine > 1.0 - 1e-9)
			return glm::dquat(1.0, 0.0, 0.0, 0.0);
		if (cosine < -1.0 + 1e-9)
			return glm::angleAxis(glm::pi<double>(), glm::dvec3(1.0, 0.0, 0.0));

		return glm::angleAxis(std::acos(cosine), glm::normalize(glm::cross(yAxis, target)));
	}

	void appendRod(MeshData& destination, const glm::dvec3& start, const glm::dvec3& end,
		double radius, unsigned int segments = 6)
	{
		const glm::dvec3 delta = end - start;
		const double length = glm::length(delta);
		// The scientific scene scale makes a 13 m boom about 8.7e-9 world
		// units long. Only reject a genuinely zero-length construction edge;
		// a presentation-scale epsilon would silently remove the trusses.
		if (length <= 1e-16)
			return;

		const MeshData cylinder = CylinderGenerator::generate(
			static_cast<float>(radius), static_cast<float>(radius), static_cast<float>(length), segments);
		const glm::dmat4 transform = glm::translate(glm::dmat4(1.0), (start + end) * 0.5)
			* glm::mat4_cast(rotateYTo(delta));
		appendTransformed(destination, cylinder, transform);
	}

	MeshData buildTriangularTruss(const glm::dvec3& start, const glm::dvec3& end,
		double halfWidth, unsigned int bays, double rodRadius)
	{
		MeshData result;
		const glm::dvec3 axis = glm::normalize(end - start);
		const glm::dvec3 reference = std::abs(axis.y) < 0.9
			? glm::dvec3(0.0, 1.0, 0.0) : glm::dvec3(1.0, 0.0, 0.0);
		const glm::dvec3 sideA = glm::normalize(glm::cross(axis, reference));
		const glm::dvec3 sideB = glm::normalize(glm::cross(axis, sideA));
		const std::array<glm::dvec3, 3> offsets = {
			sideA * halfWidth,
			(-0.5 * sideA + 0.866025403784 * sideB) * halfWidth,
			(-0.5 * sideA - 0.866025403784 * sideB) * halfWidth
		};

		for (const glm::dvec3& offset : offsets)
			appendRod(result, start + offset, end + offset, rodRadius);

		for (unsigned int bay = 0; bay < bays; ++bay)
		{
			const double t0 = static_cast<double>(bay) / static_cast<double>(bays);
			const double t1 = static_cast<double>(bay + 1) / static_cast<double>(bays);
			const glm::dvec3 center0 = glm::mix(start, end, t0);
			const glm::dvec3 center1 = glm::mix(start, end, t1);
			for (std::size_t rail = 0; rail < offsets.size(); ++rail)
			{
				const std::size_t next = (rail + 1) % offsets.size();
				// One diagonal per face per bay. Alternating the direction forms
				// the characteristic repeating lattice without doubled/coplanar rods.
				if ((bay + static_cast<unsigned int>(rail)) % 2 == 0)
					appendRod(result, center0 + offsets[rail], center1 + offsets[next], rodRadius * 0.75);
				else
					appendRod(result, center0 + offsets[next], center1 + offsets[rail], rodRadius * 0.75);
			}
		}

		return result;
	}

	std::unique_ptr<SceneObject> makePart(const std::string& name, const std::shared_ptr<Mesh>& mesh,
		const std::shared_ptr<Material>& material, const glm::dvec3& position = glm::dvec3(0.0),
		const glm::dquat& rotation = glm::dquat(1.0, 0.0, 0.0, 0.0),
		const glm::dvec3& scale = glm::dvec3(1.0))
	{
		auto part = std::make_unique<SceneObject>(name);
		part->setMesh(mesh);
		part->setMaterial(material);
		part->transform().position = position;
		part->transform().rotation = rotation;
		part->transform().scale = scale;
		return part;
	}

	std::shared_ptr<Mesh> upload(const MeshData& data)
	{
		return std::make_shared<Mesh>(data);
	}
}


VoyagerModelBuildResult VoyagerModelBuilder::build()
{
	VoyagerModelBuildResult result;
	result.spacecraft = std::make_unique<Voyager2>("voyager2");
	Voyager2& voyager = *result.spacecraft;

	// Surface finishes: photographs of real Voyager hardware from NASA's
	// public-domain texture atlas (assets/textures/spacecraft/README.md),
	// each part showing one rectangle of it, plus the specular response of
	// that finish. Rectangles are (left, top, right, bottom) atlas pixels.
	// Multi-layer insulation foil is bright and mirror-like; paint is matte.
	const MaterialLibrary::Atlas atlas =
		MaterialLibrary::loadAtlas("assets/textures/spacecraft/voyager_nasa_atlas.png", 1.4f);
	auto finish = [&atlas](const glm::vec4& rect, const glm::vec3& tint, float specular, float power)
	{
		return MaterialLibrary::spacecraftTextured(atlas, rect, tint, specular, power);
	};
	const auto whitePaint = finish({ 225, 835, 352, 958 }, glm::vec3(1.0f), 0.20f, 16.0f);
	const auto goldFoil = finish({ 390, 710, 490, 818 }, glm::vec3(1.15f), 0.75f, 70.0f);
	const auto darkGoldFoil = finish({ 390, 710, 490, 818 }, glm::vec3(0.62f), 0.55f, 50.0f);
	const auto blackBlanket = finish({ 8, 8, 212, 160 }, glm::vec3(1.0f), 0.12f, 10.0f);
	const auto aluminium = finish({ 978, 20, 1010, 560 }, glm::vec3(1.1f), 0.55f, 40.0f);
	const auto darkMetal = finish({ 150, 492, 232, 626 }, glm::vec3(0.45f), 0.35f, 30.0f);
	const auto lens = finish({ 298, 672, 360, 736 }, glm::vec3(1.0f), 0.90f, 120.0f);
	const auto copper = MaterialLibrary::spacecraft(glm::vec3(0.72f, 0.36f, 0.12f), 0.60f, 48.0f);
	const auto radiatorBlue = finish({ 480, 356, 640, 536 }, glm::vec3(1.0f), 0.30f, 24.0f);
	const auto recordGold = finish({ 8, 172, 234, 396 }, glm::vec3(1.1f), 0.85f, 90.0f);
	const auto louvres = finish({ 278, 122, 448, 280 }, glm::vec3(1.0f), 0.60f, 50.0f);
	const auto calibrationPanel = finish({ 470, 20, 630, 220 }, glm::vec3(1.0f), 0.15f, 12.0f);

	const ScaleManager scale;
	auto units = [&scale](double metres)
	{
		return scale.spacecraftSizeToRenderUnits(metres);
	};
	auto fl = [](double value) { return static_cast<float>(value); };

	auto add = [&](std::unique_ptr<SceneObject> part, std::size_t triangles)
	{
		voyager.addChild(std::move(part));
		++result.visiblePartCount;
		result.renderedTriangleCount += triangles;
	};
	// Component registry for Inspect mode: a name, a one-line fact and the
	// point (in metres, spacecraft frame) the camera should orbit.
	auto component = [&](const std::string& name, const std::string& description,
		const glm::dvec3& centre, double sizeMetres)
	{
		voyager.addComponent({ name, description, centre, units(sizeMetres) });
	};

	const MeshData unitBoxData = BoxGenerator::generate();
	const auto unitBox = upload(unitBoxData);
	auto addBox = [&](const std::string& name, const glm::dvec3& position, const glm::dvec3& dimensions,
		const std::shared_ptr<Material>& material, const glm::dquat& rotation = glm::dquat(1.0, 0.0, 0.0, 0.0))
	{
		add(makePart(name, unitBox, material, position, rotation, dimensions), unitBoxData.indices.size() / 3);
	};
	// A cylinder (or frustum) centred at `centre` whose axis points along `axis`.
	auto addCylinder = [&](const std::string& name, const glm::dvec3& centre, const glm::dvec3& axis,
		double bottomRadius, double topRadius, double length, unsigned int segments,
		const std::shared_ptr<Material>& material)
	{
		const MeshData data = CylinderGenerator::generate(fl(bottomRadius), fl(topRadius), fl(length), segments);
		add(makePart(name, upload(data), material, centre, rotateYTo(axis)), data.indices.size() / 3);
	};

	// ------------------------------------------------------------------
	// Bus: decagonal, 1.78 m across the flats' circle, 0.47 m deep, axis +Z.
	// ------------------------------------------------------------------
	const double busRadius = units(1.78) * 0.5;
	const double busDepth = units(0.47);
	const MeshData busData = CylinderGenerator::generate(fl(busRadius), fl(busRadius), fl(busDepth), 10);
	add(makePart("voyager2_bus", upload(busData), darkMetal, glm::dvec3(0.0),
		glm::angleAxis(glm::radians(90.0), glm::dvec3(1.0, 0.0, 0.0))), busData.indices.size() / 3);

	// Ten electronics bays, one per face, each covered by a thermal blanket.
	// Face k has its centre at angle (k + 0.5) * 36 degrees and sits at the
	// apothem R cos(18 deg); the blanket is a thin box slightly proud of it.
	const double apothem = busRadius * std::cos(glm::radians(18.0));
	const double faceWidth = 2.0 * busRadius * std::sin(glm::radians(18.0));
	for (int face = 0; face < 10; ++face)
	{
		const double angle = glm::radians((face + 0.5) * 36.0);
		const glm::dvec3 outward(std::cos(angle), std::sin(angle), 0.0);
		const glm::dquat facing = glm::angleAxis(angle, glm::dvec3(0.0, 0.0, 1.0));
		const auto& blanket = face % 3 == 0 ? darkGoldFoil : (face % 3 == 1 ? goldFoil : blackBlanket);
		addBox("voyager2_bay_" + std::to_string(face), outward * (apothem + units(0.012)),
			{ units(0.02), faceWidth * 0.86, busDepth * 0.84 }, blanket, facing);
		// Two thermal louvre strips on the gold bays.
		if (face % 3 == 1)
		{
			for (int louvre = -1; louvre <= 1; louvre += 2)
			{
				addBox("voyager2_louvre_" + std::to_string(face) + "_" + std::to_string(louvre),
					outward * (apothem + units(0.03)) + glm::dvec3(0.0, 0.0, louvre * units(0.09)),
					{ units(0.012), faceWidth * 0.6, units(0.05) }, louvres, facing);
			}
		}
	}
	component("Electronics bus", "Ten-sided bus, 1.78 m wide and 0.47 m deep; one bay of electronics behind each face.",
		glm::dvec3(0.0), 1.2);

	// Launch-adapter feet: the three struts under the bus (+Z side).
	MeshData feet;
	for (int foot = 0; foot < 3; ++foot)
	{
		const double angle = glm::two_pi<double>() * foot / 3.0 + glm::radians(30.0);
		const glm::dvec3 root(busRadius * 0.55 * std::cos(angle), busRadius * 0.55 * std::sin(angle), busDepth * 0.5);
		const glm::dvec3 tip(busRadius * 0.25 * std::cos(angle), busRadius * 0.25 * std::sin(angle),
			busDepth * 0.5 + units(0.32));
		appendRod(feet, root, tip, units(0.025), 6);
	}
	add(makePart("voyager2_adapter_feet", upload(feet), aluminium), feet.indices.size() / 3);

	// ------------------------------------------------------------------
	// Antenna stack along -Z: parabolic HGA, X-band feed, frequency-selective
	// subreflector, S-band feed, low-gain antenna (JPL DESCANSO, S4).
	// ------------------------------------------------------------------
	const double dishRadius = units(3.7) * 0.5;
	const double dishDepth = units(0.38);
	const double dishRimZ = -busDepth * 0.5 - units(0.10);
	const glm::dvec3 minusZ(0.0, 0.0, -1.0);
	const MeshData dishData = ParabolicDishGenerator::generate(fl(dishRadius), fl(dishDepth), fl(units(0.045)), 64, 12);
	add(makePart("voyager2_high_gain_antenna", upload(dishData), whitePaint, glm::dvec3(0.0, 0.0, dishRimZ),
		glm::angleAxis(glm::radians(-90.0), glm::dvec3(1.0, 0.0, 0.0))), dishData.indices.size() / 3);
	component("High-gain antenna", "3.7 m parabolic reflector; X- and S-band link to Earth, boresight along -Z.",
		glm::dvec3(0.0, 0.0, dishRimZ), 2.2);

	// Radial ribs on the back of the dish (the reflector's support frame).
	MeshData ribs;
	for (int rib = 0; rib < 12; ++rib)
	{
		const double angle = glm::two_pi<double>() * rib / 12.0;
		const glm::dvec3 radial(std::cos(angle), std::sin(angle), 0.0);
		const glm::dvec3 hub = glm::dvec3(0.0, 0.0, dishRimZ + dishDepth * 0.9);
		const glm::dvec3 rim = radial * (dishRadius * 0.97) + glm::dvec3(0.0, 0.0, dishRimZ + units(0.03));
		appendRod(ribs, hub, rim, units(0.02), 5);
	}
	add(makePart("voyager2_hga_ribs", upload(ribs), aluminium), ribs.indices.size() / 3);

	const double xFeedZ = dishRimZ - units(0.42);
	const double subreflectorZ = dishRimZ - units(0.74);
	const double sFeedZ = dishRimZ - units(0.90);
	const double lowGainZ = dishRimZ - units(1.08);
	addCylinder("voyager2_x_band_feed", glm::dvec3(0.0, 0.0, xFeedZ), minusZ, units(0.11), units(0.07), units(0.36), 16, darkMetal);
	addCylinder("voyager2_subreflector", glm::dvec3(0.0, 0.0, subreflectorZ), minusZ, units(0.29), units(0.29), units(0.035), 32, whitePaint);
	addCylinder("voyager2_s_band_feed", glm::dvec3(0.0, 0.0, sFeedZ), minusZ, units(0.12), units(0.07), units(0.26), 16, aluminium);
	addCylinder("voyager2_low_gain_antenna", glm::dvec3(0.0, 0.0, lowGainZ), minusZ, units(0.10), units(0.025), units(0.18), 16, whitePaint);
	component("Feed stack and low-gain antenna", "X-band feed, frequency-selective subreflector, S-band feed and the wide-beam low-gain antenna.",
		glm::dvec3(0.0, 0.0, subreflectorZ), 0.7);

	// Four struts from the dish rim to the subreflector.
	MeshData feedSupports;
	for (int support = 0; support < 4; ++support)
	{
		const double angle = glm::two_pi<double>() * support / 4.0 + glm::radians(45.0);
		const glm::dvec3 rimPoint(dishRadius * 0.78 * std::cos(angle), dishRadius * 0.78 * std::sin(angle),
			dishRimZ + dishDepth * 0.35);
		appendRod(feedSupports, rimPoint, glm::dvec3(0.0, 0.0, subreflectorZ), units(0.018), 6);
	}
	add(makePart("voyager2_hga_feed_supports", upload(feedSupports), aluminium), feedSupports.indices.size() / 3);

	// Sun sensor on the dish rim, looking along the boresight.
	const glm::dvec3 sunSensor(0.0, dishRadius * 0.93, dishRimZ - units(0.06));
	addBox("voyager2_sun_sensor", sunSensor, { units(0.12), units(0.08), units(0.12) }, darkMetal);
	addCylinder("voyager2_sun_sensor_aperture", sunSensor + glm::dvec3(0.0, 0.0, -units(0.07)), minusZ,
		units(0.035), units(0.035), units(0.03), 12, lens);
	component("Sun sensor", "Keeps the antenna pointed: the Sun and Earth lie close together as seen from the outer planets.",
		sunSensor, 0.35);

	// ------------------------------------------------------------------
	// Golden Record, calibration target and radiator on the bus faces.
	// ------------------------------------------------------------------
	const double recordAngle = glm::radians(0.5 * 36.0);
	const glm::dvec3 recordOutward(std::cos(recordAngle), std::sin(recordAngle), 0.0);
	addCylinder("voyager2_golden_record", recordOutward * (apothem + units(0.04)), recordOutward,
		units(0.155), units(0.155), units(0.012), 32, recordGold);
	addCylinder("voyager2_record_hub", recordOutward * (apothem + units(0.05)), recordOutward,
		units(0.03), units(0.03), units(0.012), 12, aluminium);
	component("Golden Record", "30 cm gold-plated copper phonograph record with sounds and images of Earth.",
		recordOutward * apothem, 0.45);

	const double targetAngle = glm::radians(5.5 * 36.0);
	const glm::dvec3 targetOutward(std::cos(targetAngle), std::sin(targetAngle), 0.0);
	addBox("voyager2_calibration_target", targetOutward * (apothem + units(0.04)),
		{ units(0.03), units(0.42), units(0.34) }, calibrationPanel, glm::angleAxis(targetAngle, glm::dvec3(0.0, 0.0, 1.0)));
	component("Optical calibration target", "Flat plate of known colour the cameras photograph to calibrate themselves.",
		targetOutward * apothem, 0.5);

	const double radiatorAngle = glm::radians(7.5 * 36.0);
	const glm::dvec3 radiatorOutward(std::cos(radiatorAngle), std::sin(radiatorAngle), 0.0);
	addBox("voyager2_shunt_radiator", radiatorOutward * (apothem + units(0.04)),
		{ units(0.03), units(0.46), units(0.38) }, radiatorBlue, glm::angleAxis(radiatorAngle, glm::dvec3(0.0, 0.0, 1.0)));

	// ------------------------------------------------------------------
	// Magnetometer boom: 13 m Astromast lattice from a launch canister, with
	// two high-field sensors near the base and low-field sensors at 10 m and
	// at the 13 m tip (PDS host description, S6).
	// ------------------------------------------------------------------
	const glm::dvec3 magnetometerStart(busRadius, units(0.12), units(0.08));
	const glm::dvec3 magnetometerDirection = glm::normalize(glm::dvec3(1.0, 0.10, 0.05));
	const glm::dvec3 magnetometerEnd = magnetometerStart + magnetometerDirection * units(13.0);
	addCylinder("voyager2_magnetometer_canister", magnetometerStart + magnetometerDirection * units(0.3),
		magnetometerDirection, units(0.2), units(0.2), units(0.6), 16, darkGoldFoil);
	const MeshData magnetometerTruss = buildTriangularTruss(magnetometerStart + magnetometerDirection * units(0.6),
		magnetometerEnd, units(0.055), 26, units(0.010));
	add(makePart("voyager2_magnetometer_boom", upload(magnetometerTruss), aluminium), magnetometerTruss.indices.size() / 3);
	for (double station : { 0.9, 1.4 })
		addBox("voyager2_high_field_magnetometer", magnetometerStart + magnetometerDirection * units(station),
			glm::dvec3(units(0.14)), whitePaint, rotateYTo(magnetometerDirection));
	addBox("voyager2_low_field_magnetometer_inner", magnetometerStart + magnetometerDirection * units(10.0),
		glm::dvec3(units(0.2)), whitePaint, rotateYTo(magnetometerDirection));
	addBox("voyager2_low_field_magnetometer_tip", magnetometerEnd, glm::dvec3(units(0.24)), whitePaint,
		rotateYTo(magnetometerDirection));
	component("Magnetometer boom", "13 m Astromast lattice; keeps the magnetometers far from the spacecraft's own fields.",
		magnetometerStart + magnetometerDirection * units(6.5), 7.5);
	component("Low-field magnetometer (tip)", "Measures the interplanetary and planetary magnetic fields at the 13 m tip.",
		magnetometerEnd, 0.6);

	// ------------------------------------------------------------------
	// RTG boom and three MHW-RTGs (0.58 m long, 0.40 m across six fins).
	// ------------------------------------------------------------------
	const glm::dvec3 rtgStart(-busRadius, -units(0.12), units(0.04));
	const glm::dvec3 rtgDirection = glm::normalize(glm::dvec3(-1.0, -0.20, 0.05));
	const double rtgBoomLength = units(3.7);
	const MeshData rtgTruss = buildTriangularTruss(rtgStart, rtgStart + rtgDirection * rtgBoomLength,
		units(0.05), 9, units(0.013));
	add(makePart("voyager2_rtg_boom", upload(rtgTruss), aluminium), rtgTruss.indices.size() / 3);

	MeshData rtgAssembly;
	MeshData rtgCaps;
	const glm::dvec3 rtgReference = std::abs(rtgDirection.y) < 0.9 ? glm::dvec3(0.0, 1.0, 0.0) : glm::dvec3(0.0, 0.0, 1.0);
	const glm::dvec3 rtgSideA = glm::normalize(glm::cross(rtgDirection, rtgReference));
	const glm::dvec3 rtgSideB = glm::normalize(glm::cross(rtgDirection, rtgSideA));
	for (int generator = 0; generator < 3; ++generator)
	{
		const double startDistance = rtgBoomLength + units(generator * 0.64);
		const double endDistance = startDistance + units(0.58);
		appendRod(rtgAssembly, rtgStart + rtgDirection * startDistance, rtgStart + rtgDirection * endDistance,
			units(0.10), 16);
		// End flanges between units.
		appendRod(rtgCaps, rtgStart + rtgDirection * (startDistance - units(0.02)),
			rtgStart + rtgDirection * (startDistance + units(0.02)), units(0.13), 16);
		appendRod(rtgCaps, rtgStart + rtgDirection * (endDistance - units(0.02)),
			rtgStart + rtgDirection * (endDistance + units(0.02)), units(0.13), 16);
		const glm::dvec3 center = rtgStart + rtgDirection * ((startDistance + endDistance) * 0.5);
		for (int fin = 0; fin < 6; ++fin)
		{
			const double angle = glm::two_pi<double>() * fin / 6.0;
			const glm::dvec3 radial = std::cos(angle) * rtgSideA + std::sin(angle) * rtgSideB;
			const glm::dvec3 tangent = glm::normalize(glm::cross(rtgDirection, radial));
			glm::dmat4 finTransform(1.0);
			finTransform[0] = glm::dvec4(rtgDirection * units(0.50), 0.0);
			finTransform[1] = glm::dvec4(radial * units(0.10), 0.0);
			finTransform[2] = glm::dvec4(tangent * units(0.02), 0.0);
			finTransform[3] = glm::dvec4(center + radial * units(0.15), 1.0);
			appendTransformed(rtgAssembly, unitBoxData, finTransform);
		}
	}
	add(makePart("voyager2_three_rtgs", upload(rtgAssembly), darkMetal), rtgAssembly.indices.size() / 3);
	add(makePart("voyager2_rtg_flanges", upload(rtgCaps), aluminium), rtgCaps.indices.size() / 3);
	component("Radioisotope generators", "Three plutonium-238 RTGs in tandem; about 470 W at launch, still powering Voyager today.",
		rtgStart + rtgDirection * (rtgBoomLength + units(0.9)), 1.6);

	// ------------------------------------------------------------------
	// Science boom (3 m) with inboard particle instruments and the
	// two-axis scan platform at its tip.
	// ------------------------------------------------------------------
	const glm::dvec3 scienceStart(busRadius * 0.65, busRadius * 0.70, units(0.04));
	const glm::dvec3 scienceDirection = glm::normalize(glm::dvec3(3.0, 0.30, 0.15));
	const glm::dvec3 scienceEnd = scienceStart + scienceDirection * units(3.0);
	const MeshData scienceTruss = buildTriangularTruss(scienceStart, scienceEnd, units(0.05), 7, units(0.013));
	add(makePart("voyager2_science_boom", upload(scienceTruss), aluminium), scienceTruss.indices.size() / 3);

	// Plasma Science: a cylinder with sensor cups looking out along the boom.
	const glm::dvec3 plasma = scienceStart + scienceDirection * units(0.9) + glm::dvec3(0.0, units(0.22), 0.0);
	addCylinder("voyager2_plasma_science", plasma, glm::dvec3(0.0, 1.0, 0.0), units(0.16), units(0.16), units(0.22), 16, goldFoil);
	for (int cup = 0; cup < 3; ++cup)
	{
		const double angle = glm::two_pi<double>() * cup / 3.0;
		const glm::dvec3 cupDirection = glm::normalize(glm::dvec3(std::cos(angle), 1.2, std::sin(angle)));
		addCylinder("voyager2_plasma_cup_" + std::to_string(cup), plasma + cupDirection * units(0.16), cupDirection,
			units(0.05), units(0.07), units(0.08), 12, lens);
	}
	component("Plasma science instrument", "Faraday cups measuring the solar wind's speed, density and temperature.",
		plasma, 0.5);

	// Cosmic Ray Subsystem: a box of telescopes on the boom.
	const glm::dvec3 cosmicRay = scienceStart + scienceDirection * units(1.6) + glm::dvec3(0.0, units(0.2), 0.0);
	addBox("voyager2_cosmic_ray_subsystem", cosmicRay, { units(0.42), units(0.28), units(0.30) }, goldFoil);
	for (int telescope = 0; telescope < 2; ++telescope)
		addCylinder("voyager2_cosmic_ray_telescope_" + std::to_string(telescope),
			cosmicRay + glm::dvec3((telescope == 0 ? -1.0 : 1.0) * units(0.1), units(0.17), 0.0), glm::dvec3(0.0, 1.0, 0.0),
			units(0.05), units(0.05), units(0.07), 12, darkMetal);
	component("Cosmic ray subsystem", "Telescopes counting high-energy particles; it detected the heliopause crossing in 2018.",
		cosmicRay, 0.5);

	// Low-Energy Charged Particle instrument: a drum on a stepper platform.
	const glm::dvec3 lecp = scienceStart + scienceDirection * units(2.25) + glm::dvec3(0.0, -units(0.22), 0.0);
	addCylinder("voyager2_lecp_drum", lecp, glm::dvec3(0.0, -1.0, 0.0), units(0.15), units(0.15), units(0.30), 20, darkGoldFoil);
	addCylinder("voyager2_lecp_platform", lecp + glm::dvec3(0.0, units(0.18), 0.0), glm::dvec3(0.0, 1.0, 0.0),
		units(0.19), units(0.19), units(0.04), 20, aluminium);
	component("Low-energy charged particles", "A rotating drum of detectors sampling ions and electrons from every direction.",
		lecp, 0.5);

	// Scan platform: the articulated head that points the remote-sensing
	// instruments. Cameras and spectrometers all look the same way (+X/-Z).
	addBox("voyager2_scan_platform", scienceEnd, { units(0.60), units(0.46), units(0.40) }, darkGoldFoil);
	addCylinder("voyager2_scan_actuator", scienceEnd - scienceDirection * units(0.32), scienceDirection,
		units(0.12), units(0.12), units(0.18), 16, aluminium);
	const glm::dvec3 lookDirection = glm::normalize(glm::dvec3(0.25, -0.35, -1.0));
	const glm::dvec3 narrowAngle = scienceEnd + glm::dvec3(-units(0.18), -units(0.30), -units(0.20));
	addCylinder("voyager2_narrow_angle_camera", narrowAngle + lookDirection * units(0.30), lookDirection,
		units(0.13), units(0.13), units(0.95), 20, whitePaint);
	addCylinder("voyager2_narrow_angle_lens", narrowAngle + lookDirection * units(0.79), lookDirection,
		units(0.10), units(0.10), units(0.02), 20, lens);
	const glm::dvec3 wideAngle = scienceEnd + glm::dvec3(units(0.14), -units(0.30), -units(0.20));
	addCylinder("voyager2_wide_angle_camera", wideAngle + lookDirection * units(0.12), lookDirection,
		units(0.10), units(0.10), units(0.50), 20, whitePaint);
	addCylinder("voyager2_wide_angle_lens", wideAngle + lookDirection * units(0.38), lookDirection,
		units(0.075), units(0.075), units(0.02), 20, lens);
	const glm::dvec3 iris = scienceEnd + glm::dvec3(units(0.02), units(0.34), -units(0.15));
	addCylinder("voyager2_iris_telescope", iris, lookDirection, units(0.26), units(0.26), units(0.40), 24, goldFoil);
	addCylinder("voyager2_iris_mirror", iris + lookDirection * units(0.21), lookDirection, units(0.22), units(0.22),
		units(0.015), 24, lens);
	addBox("voyager2_ultraviolet_spectrometer", scienceEnd + glm::dvec3(units(0.40), units(0.05), 0.0),
		{ units(0.22), units(0.30), units(0.46) }, blackBlanket);
	addCylinder("voyager2_photopolarimeter", scienceEnd + glm::dvec3(-units(0.42), units(0.10), -units(0.12)),
		lookDirection, units(0.07), units(0.07), units(0.32), 16, aluminium);
	component("Scan platform", "Two-axis platform aiming the cameras, IRIS, ultraviolet spectrometer and photopolarimeter.",
		scienceEnd, 1.0);
	component("Narrow-angle camera", "1500 mm f/8.5 imaging science camera: most close-up planetary images came from here.",
		narrowAngle + lookDirection * units(0.3), 0.6);
	component("Wide-angle camera", "200 mm camera for wide context views and colour mosaics.",
		wideAngle + lookDirection * units(0.12), 0.45);
	component("IRIS telescope", "Infrared interferometer spectrometer behind a 0.5 m Cassegrain telescope: temperatures and composition.",
		iris, 0.6);

	// ------------------------------------------------------------------
	// Planetary radio astronomy / plasma wave antennas: two 10 m elements in
	// a V from a shared root box.
	// ------------------------------------------------------------------
	MeshData plasmaAntennas;
	const glm::dvec3 antennaRoot(0.0, -busRadius * 0.85, units(0.04));
	appendRod(plasmaAntennas, antennaRoot, antennaRoot + glm::normalize(glm::dvec3(-0.75, -1.0, 0.15)) * units(10.0),
		units(0.012), 6);
	appendRod(plasmaAntennas, antennaRoot, antennaRoot + glm::normalize(glm::dvec3(0.75, -1.0, -0.15)) * units(10.0),
		units(0.012), 6);
	add(makePart("voyager2_plasma_wave_antennas", upload(plasmaAntennas), aluminium), plasmaAntennas.indices.size() / 3);
	addBox("voyager2_pra_root", antennaRoot, { units(0.22), units(0.14), units(0.18) }, goldFoil);
	component("Radio and plasma wave antennas", "Two 10 m whips in a V: they heard lightning at Uranus and plasma waves at the heliopause.",
		antennaRoot, 1.2);

	// ------------------------------------------------------------------
	// Sixteen hydrazine thrusters in four clusters around the bus.
	// ------------------------------------------------------------------
	const MeshData thrusterData = CylinderGenerator::generate(fl(units(0.07)), fl(units(0.03)), fl(units(0.14)), 12);
	const auto thrusterMesh = upload(thrusterData);
	for (int cluster = 0; cluster < 4; ++cluster)
	{
		const double angle = glm::half_pi<double>() * cluster + glm::radians(9.0);
		const glm::dvec3 radial(std::cos(angle), std::sin(angle), 0.0);
		const glm::dquat radialRotation = rotateYTo(radial);
		const glm::dvec3 tangent(-std::sin(angle), std::cos(angle), 0.0);
		addBox("voyager2_thruster_block_" + std::to_string(cluster), radial * (busRadius + units(0.05)),
			{ units(0.08), units(0.20), units(0.36) }, darkMetal, glm::angleAxis(angle, glm::dvec3(0.0, 0.0, 1.0)));
		for (int nozzle = 0; nozzle < 4; ++nozzle)
		{
			const double axialSign = nozzle < 2 ? -1.0 : 1.0;
			const double tangentSign = nozzle % 2 == 0 ? -1.0 : 1.0;
			const glm::dvec3 position = radial * (busRadius + units(0.14)) + tangent * (tangentSign * units(0.06))
				+ glm::dvec3(0.0, 0.0, axialSign * units(0.12));
			add(makePart("voyager2_thruster_" + std::to_string(cluster * 4 + nozzle), thrusterMesh, copper, position,
				radialRotation), thrusterData.indices.size() / 3);
		}
	}
	component("Attitude thrusters", "Sixteen small hydrazine thrusters in four clusters; they turn the craft and trimmed its trajectory.",
		glm::dvec3(busRadius + units(0.14), 0.0, 0.0), 0.5);

	return result;
}
