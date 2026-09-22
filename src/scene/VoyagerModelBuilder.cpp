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
#include "../rendering/Mesh.h"
#include "../rendering/ParabolicDishGenerator.h"

namespace
{
	std::shared_ptr<Material> makeMaterial(const glm::vec3& color)
	{
		auto material = std::make_shared<Material>();
		material->baseColor = color;
		return material;
	}

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

	const auto white = makeMaterial(glm::vec3(0.93f, 0.93f, 0.90f));
	const auto gold = makeMaterial(glm::vec3(0.78f, 0.57f, 0.16f));
	const auto darkGold = makeMaterial(glm::vec3(0.40f, 0.27f, 0.08f));
	const auto metal = makeMaterial(glm::vec3(0.58f, 0.61f, 0.64f));
	const auto darkMetal = makeMaterial(glm::vec3(0.16f, 0.18f, 0.20f));
	const auto black = makeMaterial(glm::vec3(0.035f, 0.045f, 0.055f));
	const auto copper = makeMaterial(glm::vec3(0.72f, 0.28f, 0.08f));
	const auto blueGrey = makeMaterial(glm::vec3(0.35f, 0.48f, 0.55f));

	const ScaleManager scale;
	auto units = [&scale](double metres)
	{
		return scale.spacecraftSizeToRenderUnits(metres);
	};

	// NASA dimensions: 1.78 m decagonal bus, 0.47 m deep; 3.7 m HGA.
	// All major parts use one linear scale, so the dish is correctly a little
	// over twice the bus diameter instead of the earlier inverted ratio.
	const double busRadius = units(1.78) * 0.5;
	const double busDepth = units(0.47);
	const double dishRadius = units(3.7) * 0.5;
	const double dishDepth = units(0.38);
	const double dishThickness = units(0.045);
	const double dishRimZ = -busDepth * 0.5 - units(0.10);

	auto add = [&](std::unique_ptr<SceneObject> part, std::size_t triangles)
	{
		voyager.addChild(std::move(part));
		++result.visiblePartCount;
		result.renderedTriangleCount += triangles;
	};

	const MeshData busData = CylinderGenerator::generate(
		static_cast<float>(busRadius), static_cast<float>(busRadius), static_cast<float>(busDepth), 10);
	add(makePart("voyager2_bus", upload(busData), gold, glm::dvec3(0.0),
		glm::angleAxis(glm::radians(90.0), glm::dvec3(1.0, 0.0, 0.0))), busData.indices.size() / 3);

	const MeshData dishData = ParabolicDishGenerator::generate(
		static_cast<float>(dishRadius), static_cast<float>(dishDepth),
		static_cast<float>(dishThickness), 48, 8);
	add(makePart("voyager2_high_gain_antenna", upload(dishData), white,
		glm::dvec3(0.0, 0.0, dishRimZ),
		glm::angleAxis(glm::radians(-90.0), glm::dvec3(1.0, 0.0, 0.0))), dishData.indices.size() / 3);

	// HGA feed horn and its three non-coplanar support struts are visible in
	// front/three-quarter views and are essential to reading this as a dish,
	// rather than a plain white disc.
	const double feedZ = dishRimZ - units(0.72);
	const MeshData feedData = CylinderGenerator::generate(
		static_cast<float>(units(0.07)), static_cast<float>(units(0.16)),
		static_cast<float>(units(0.30)), 12);
	add(makePart("voyager2_hga_feed", upload(feedData), darkMetal,
		glm::dvec3(0.0, 0.0, feedZ),
		glm::angleAxis(glm::radians(-90.0), glm::dvec3(1.0, 0.0, 0.0))), feedData.indices.size() / 3);
	const MeshData lowGainData = CylinderGenerator::generate(
		static_cast<float>(units(0.10)), static_cast<float>(units(0.025)),
		static_cast<float>(units(0.20)), 12);
	add(makePart("voyager2_low_gain_antenna", upload(lowGainData), white,
		glm::dvec3(0.0, 0.0, feedZ - units(0.24)),
		glm::angleAxis(glm::radians(-90.0), glm::dvec3(1.0, 0.0, 0.0))), lowGainData.indices.size() / 3);

	MeshData feedSupports;
	for (int support = 0; support < 3; ++support)
	{
		const double angle = glm::two_pi<double>() * static_cast<double>(support) / 3.0;
		const glm::dvec3 rimPoint(
			dishRadius * 0.68 * std::cos(angle), dishRadius * 0.68 * std::sin(angle), dishRimZ - units(0.02));
		appendRod(feedSupports, rimPoint, glm::dvec3(0.0, 0.0, feedZ), units(0.018), 5);
	}
	add(makePart("voyager2_hga_feed_supports", upload(feedSupports), metal), feedSupports.indices.size() / 3);

	const MeshData unitBoxData = BoxGenerator::generate();
	const auto unitBox = upload(unitBoxData);
	auto addBox = [&](const std::string& name, const glm::dvec3& position,
		const glm::dvec3& dimensions, const std::shared_ptr<Material>& material,
		const glm::dquat& rotation = glm::dquat(1.0, 0.0, 0.0, 0.0))
	{
		add(makePart(name, unitBox, material, position, rotation, dimensions), unitBoxData.indices.size() / 3);
	};

	// Bus-mounted radiator, electronics bay and optical calibration target.
	// Small offsets keep each panel above the bus skin and prevent z-fighting.
	addBox("voyager2_shunt_radiator", { -busRadius - units(0.025), 0.0, units(0.02) },
		{ units(0.05), units(0.72), units(0.62) }, blueGrey);
	addBox("voyager2_electronics_bay", { 0.0, busRadius + units(0.025), units(0.01) },
		{ units(0.66), units(0.05), units(0.50) }, darkGold);
	addBox("voyager2_calibration_target", { busRadius + units(0.03), 0.0, -units(0.04) },
		{ units(0.05), units(0.50), units(0.38) }, white);

	const MeshData recordData = CylinderGenerator::generate(
		static_cast<float>(units(0.30)), static_cast<float>(units(0.30)), static_cast<float>(units(0.025)), 24);
	add(makePart("voyager2_golden_record", upload(recordData), gold,
		glm::dvec3(busRadius + units(0.055), 0.0, 0.0),
		glm::angleAxis(glm::radians(-90.0), glm::dvec3(0.0, 0.0, 1.0))), recordData.indices.size() / 3);

	// 13 m Astromast: a three-rail lattice with alternating braces, not a
	// solid oversized rod. The two sensor packages sit at the documented
	// 7 m and 13 m stations.
	const glm::dvec3 magnetometerStart(busRadius, units(0.12), units(0.08));
	const glm::dvec3 magnetometerDirection = glm::normalize(glm::dvec3(1.0, 0.10, 0.05));
	const glm::dvec3 magnetometerEnd = magnetometerStart + magnetometerDirection * units(13.0);
	const MeshData magnetometerTruss = buildTriangularTruss(
		magnetometerStart, magnetometerEnd, units(0.055), 18, units(0.010));
	add(makePart("voyager2_magnetometer_boom", upload(magnetometerTruss), metal),
		magnetometerTruss.indices.size() / 3);
	const glm::dvec3 midSensor = magnetometerStart + magnetometerDirection * units(7.0);
	addBox("voyager2_mid_field_magnetometer", midSensor,
		glm::dvec3(units(0.18)), darkMetal, rotateYTo(magnetometerDirection));
	addBox("voyager2_low_field_magnetometer", magnetometerEnd,
		glm::dvec3(units(0.22)), darkMetal, rotateYTo(magnetometerDirection));

	// RTG boom and all three tandem generators. Six longitudinal radial fins
	// per generator are merged into a single mesh, keeping the real silhouette
	// without a draw call for every individual fin.
	const glm::dvec3 rtgStart(-busRadius, -units(0.12), units(0.04));
	const glm::dvec3 rtgDirection = glm::normalize(glm::dvec3(-1.0, -0.20, 0.05));
	const double rtgBoomLength = units(3.7);
	const glm::dvec3 rtgBoomEnd = rtgStart + rtgDirection * rtgBoomLength;
	const MeshData rtgTruss = buildTriangularTruss(rtgStart, rtgBoomEnd, units(0.045), 7, units(0.012));
	add(makePart("voyager2_rtg_boom", upload(rtgTruss), metal), rtgTruss.indices.size() / 3);

	MeshData rtgAssembly;
	const glm::dvec3 rtgReference = std::abs(rtgDirection.y) < 0.9
		? glm::dvec3(0.0, 1.0, 0.0) : glm::dvec3(0.0, 0.0, 1.0);
	const glm::dvec3 rtgSideA = glm::normalize(glm::cross(rtgDirection, rtgReference));
	const glm::dvec3 rtgSideB = glm::normalize(glm::cross(rtgDirection, rtgSideA));
	for (int generator = 0; generator < 3; ++generator)
	{
		const double startDistance = rtgBoomLength + units(generator * 0.64);
		const double endDistance = startDistance + units(0.58);
		appendRod(rtgAssembly, rtgStart + rtgDirection * startDistance,
			rtgStart + rtgDirection * endDistance, units(0.14), 12);
		const glm::dvec3 center = rtgStart + rtgDirection * ((startDistance + endDistance) * 0.5);
		for (int fin = 0; fin < 6; ++fin)
		{
			const double angle = glm::two_pi<double>() * static_cast<double>(fin) / 6.0;
			const glm::dvec3 radial = std::cos(angle) * rtgSideA + std::sin(angle) * rtgSideB;
			const glm::dvec3 tangent = glm::normalize(glm::cross(rtgDirection, radial));
			glm::dmat4 finTransform(1.0);
			finTransform[0] = glm::dvec4(rtgDirection * units(0.50), 0.0);
			finTransform[1] = glm::dvec4(radial * units(0.12), 0.0);
			finTransform[2] = glm::dvec4(tangent * units(0.025), 0.0);
			finTransform[3] = glm::dvec4(center + radial * units(0.14), 1.0);
			appendTransformed(rtgAssembly, unitBoxData, finTransform);
		}
	}
	add(makePart("voyager2_three_rtgs", upload(rtgAssembly), darkMetal), rtgAssembly.indices.size() / 3);

	// Science boom terminates in the articulated scan platform. Individual
	// instrument boxes and twin camera barrels make the far end asymmetric,
	// matching the real Voyager silhouette in the supplied side reference.
	const glm::dvec3 scienceStart(busRadius * 0.65, busRadius * 0.70, units(0.04));
	const glm::dvec3 scienceEnd = scienceStart + glm::dvec3(units(3.0), units(0.30), units(0.15));
	const MeshData scienceTruss = buildTriangularTruss(scienceStart, scienceEnd, units(0.045), 5, units(0.012));
	add(makePart("voyager2_science_boom", upload(scienceTruss), metal), scienceTruss.indices.size() / 3);
	addBox("voyager2_scan_platform", scienceEnd, { units(0.55), units(0.48), units(0.30) }, darkGold);
	addBox("voyager2_ultraviolet_spectrometer", scienceEnd + glm::dvec3(0.0, units(0.32), 0.0),
		{ units(0.30), units(0.18), units(0.42) }, gold);
	addBox("voyager2_infrared_spectrometer", scienceEnd + glm::dvec3(units(0.34), 0.0, units(0.02)),
		{ units(0.22), units(0.34), units(0.34) }, metal);

	const MeshData cameraBarrelData = CylinderGenerator::generate(
		static_cast<float>(units(0.12)), static_cast<float>(units(0.12)), static_cast<float>(units(0.32)), 12);
	const auto cameraBarrel = upload(cameraBarrelData);
	for (int camera = 0; camera < 2; ++camera)
	{
		add(makePart(camera == 0 ? "voyager2_wide_angle_camera" : "voyager2_narrow_angle_camera",
			cameraBarrel, black,
			scienceEnd + glm::dvec3((camera == 0 ? -1.0 : 1.0) * units(0.17), -units(0.27), -units(0.20)),
			glm::angleAxis(glm::radians(-90.0), glm::dvec3(1.0, 0.0, 0.0))),
			cameraBarrelData.indices.size() / 3);
	}

	// The shared PRA/PWS dipole is two long, thin antennas at right angles.
	MeshData plasmaAntennas;
	const glm::dvec3 antennaRoot(0.0, -busRadius * 0.75, units(0.04));
	appendRod(plasmaAntennas, antennaRoot,
		antennaRoot + glm::normalize(glm::dvec3(-0.75, -1.0, 0.15)) * units(10.0), units(0.008), 5);
	appendRod(plasmaAntennas, antennaRoot,
		antennaRoot + glm::normalize(glm::dvec3(0.75, -1.0, -0.15)) * units(10.0), units(0.008), 5);
	add(makePart("voyager2_plasma_wave_antennas", upload(plasmaAntennas), metal),
		plasmaAntennas.indices.size() / 3);

	// Four four-nozzle attitude-control thruster clusters around the bus. The
	// nozzles are shallow frusta; reusing one mesh keeps the model explicit
	// while avoiding a separate upload for all sixteen copies.
	const MeshData thrusterData = CylinderGenerator::generate(
		static_cast<float>(units(0.09)), static_cast<float>(units(0.035)), static_cast<float>(units(0.16)), 8);
	const auto thrusterMesh = upload(thrusterData);
	for (int cluster = 0; cluster < 4; ++cluster)
	{
		const double angle = glm::half_pi<double>() * static_cast<double>(cluster);
		const glm::dvec3 radial(std::cos(angle), std::sin(angle), 0.0);
		const glm::dquat radialRotation = rotateYTo(radial);
		const glm::dvec3 tangent(-std::sin(angle), std::cos(angle), 0.0);
		for (int nozzle = 0; nozzle < 4; ++nozzle)
		{
			const double axialSign = nozzle < 2 ? -1.0 : 1.0;
			const double tangentSign = nozzle % 2 == 0 ? -1.0 : 1.0;
			const glm::dvec3 position = radial * (busRadius + units(0.09))
				+ tangent * (tangentSign * units(0.055))
				+ glm::dvec3(0.0, 0.0, axialSign * units(0.16));
			add(makePart("voyager2_thruster_" + std::to_string(cluster * 4 + nozzle),
				thrusterMesh, copper, position, radialRotation), thrusterData.indices.size() / 3);
		}
	}

	return result;
}
