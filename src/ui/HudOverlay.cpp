#include "HudOverlay.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "../core/CameraController.h"
#include "../rendering/Camera.h"
#include "../rendering/TextRenderer.h"
#include "../scene/MissionController.h"
#include "../scene/SolarSystem.h"
#include "../scene/Voyager2.h"

namespace
{
	const glm::vec4 kWhite(0.92f, 0.94f, 0.97f, 1.0f);
	const glm::vec4 kAccent(1.0f, 0.78f, 0.30f, 1.0f);
	const glm::vec4 kDim(0.62f, 0.68f, 0.76f, 1.0f);

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

	std::string upper(std::string text)
	{
		std::transform(text.begin(), text.end(), text.begin(),
			[](unsigned char c) { return static_cast<char>(std::toupper(c)); });
		return text;
	}
}

std::string HudOverlay::formatJulianDate(double julianDate)
{
	const double shifted = julianDate + 0.5;
	const double z = std::floor(shifted);
	const double fraction = shifted - z;
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
	const int minutes = std::min(static_cast<int>(std::round(fraction * 1440.0)), 1439);

	std::ostringstream text;
	text << year << '-' << std::setw(2) << std::setfill('0') << month << '-' << std::setw(2) << day
		<< ' ' << std::setw(2) << minutes / 60 << ':' << std::setw(2) << minutes % 60 << " UTC";
	return text.str();
}

void HudOverlay::render(TextRenderer& text, const HudView& view)
{
	const float pixel = std::max(1.0f, std::round(static_cast<float>(view.height) / 450.0f));
	text.begin(view.width, view.height);
	if (view.hudVisible)
		renderPanel(text, view, pixel); // first: labels avoid the panel rectangle
	if (view.labelsVisible)
		renderLabels(text, view, pixel);
	if (view.hudVisible)
	{
		renderEncounterBanner(text, view, pixel);
		if (view.helpVisible)
			renderHelp(text, view, pixel);
	}
	if (view.hudVisible && !view.caption.empty())
	{
		const float big = pixel * 1.5f;
		const float width = static_cast<float>(view.width);
		const float y = static_cast<float>(view.height) * 0.72f;
		text.addShadowedText((width - TextRenderer::textWidth(view.caption, big)) * 0.5f, y, view.caption, big, kAccent);
		text.addShadowedText((width - TextRenderer::textWidth(view.captionDetail, pixel)) * 0.5f,
			y + TextRenderer::lineHeight(big), view.captionDetail, pixel, kWhite);
	}
	if (!view.notice.empty())
	{
		const float margin = pixel * 6.0f;
		text.addShadowedText(margin, static_cast<float>(view.height) - margin - TextRenderer::lineHeight(pixel),
			view.notice, pixel, kAccent);
	}
	text.flush();
}

void HudOverlay::renderLabels(TextRenderer& text, const HudView& view, float pixelSize)
{
	const Camera& camera = *view.camera;
	const SolarSystem& system = *view.system;
	const float width = static_cast<float>(view.width);
	const float height = static_cast<float>(view.height);
	const glm::mat4 viewProjection = camera.projectionMatrix(view.aspectRatio) * camera.viewMatrixAtOrigin();
	const glm::dvec3 eye = camera.position();
	const float focalPixels = height * 0.5f / std::tan(glm::radians(camera.fieldOfView()) * 0.5f);

	// Greedy declutter: labels are placed in priority order and one that
	// would overlap a placed label (or the HUD panel) is skipped.
	std::vector<glm::vec4> placed;
	if (view.hudVisible)
		placed.push_back(m_panelRect);

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

	// A name is hidden when any nearer world covers the point it labels.
	auto occluded = [&](const glm::dvec3& world, double radius)
	{
		const glm::dvec3 toTarget = world - eye;
		const double targetDistance = glm::length(toTarget);
		if (targetDistance <= 1e-12)
			return false;
		const glm::dvec3 direction = toTarget / targetDistance;
		for (const CelestialBody* blocker : system.bodies())
		{
			const glm::dmat4 blockerWorld = blocker->worldMatrix();
			const glm::dvec3 centre(blockerWorld[3]);
			const double blockerRadius = glm::length(glm::dvec3(blockerWorld[0]));
			if (glm::length(centre - world) < 1e-9)
				continue;
			const double along = glm::dot(centre - eye, direction);
			if (along <= 0.0 || along >= targetDistance - radius)
				continue;
			if (glm::length(eye + direction * along - centre) < blockerRadius)
				return true;
		}
		return false;
	};

	auto drawLabel = [&](const glm::dvec3& world, double radius, const std::string& name, const glm::vec4& color)
	{
		glm::vec2 pixel;
		if (!project(world, pixel) || occluded(world, radius))
			return;
		const double distance = glm::length(world - eye);
		const float projectedRadius = static_cast<float>(radius / std::max(distance, 1e-9)) * focalPixels;
		if (projectedRadius > height * 0.45f)
			return; // filling the screen: the name would only hide the surface
		const float labelWidth = TextRenderer::textWidth(name, pixelSize);
		const float y = pixel.y - projectedRadius - pixelSize * 11.0f;
		const glm::vec4 box(pixel.x - labelWidth * 0.5f - pixelSize, y - pixelSize,
			pixel.x + labelWidth * 0.5f + pixelSize, y + pixelSize * 8.0f);
		for (const glm::vec4& other : placed)
		{
			if (box.x < other.z && box.z > other.x && box.y < other.w && box.w > other.y)
				return;
		}
		placed.push_back(box);
		text.addShadowedText(pixel.x - labelWidth * 0.5f, y, name, pixelSize, color);
		// A small tick connects the name to a body too small to see.
		if (projectedRadius < pixelSize * 2.0f)
			text.addRect(pixel.x - pixelSize * 0.5f, y + pixelSize * 8.5f, pixelSize, pixelSize * 2.0f, color * 0.8f);
	};

	const bool inspecting = view.cameraController->mode() == CameraController::Mode::Inspect;
	auto labelBody = [&](const CelestialBody* body)
	{
		if (inspecting)
			return; // close to Voyager, only its hardware is named
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
		const CelestialBody* parent = system.find(body->data().parentId);
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
	for (const CelestialBody* body : system.bodies())
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
	const CameraController::Mode mode = view.cameraController->mode();
	if (view.voyager != nullptr && mode == CameraController::Mode::Inspect)
	{
		// Inspect mode names the spacecraft's hardware instead of worlds.
		// The whole-craft view names every component; a close-up names only
		// the one being inspected, so labels never bury the hardware.
		const auto& components = view.voyager->components();
		const int inspected = view.cameraController->inspectedComponent();
		for (std::size_t i = 0; i < components.size(); ++i)
		{
			if (inspected >= 0 && static_cast<int>(i) != inspected)
				continue;
			drawLabel(view.voyager->componentWorldCentre(i), components[i].size * 0.15, components[i].name,
				inspected >= 0 ? kAccent : glm::vec4(0.75f, 0.90f, 1.0f, 0.95f));
		}
	}
	else if (view.voyager != nullptr && mode != CameraController::Mode::Chase)
	{
		drawLabel(view.voyager->transform().position, view.voyager->boundingRadius(), "Voyager 2", kAccent);
	}
	for (const CelestialBody* body : system.bodies())
	{
		if (body->data().type == BodyType::Moon)
			labelBody(body);
	}
}

void HudOverlay::renderPanel(TextRenderer& text, const HudView& view, float pixel)
{
	const SimulationClock& clock = view.mission->clock();
	const MissionEphemeris& ephemeris = view.mission->ephemeris();
	const CameraController& controller = *view.cameraController;
	const float line = TextRenderer::lineHeight(pixel);

	std::vector<std::pair<std::string, glm::vec4>> lines;
	const double julianDate = clock.julianDate();
	lines.push_back({ "VOYAGER 2  SOLAR SYSTEM EXPLORER", kAccent });
	lines.push_back({ formatJulianDate(julianDate) + "   JD " + fixed(julianDate, 2), kWhite });

	std::string timeLine;
	if (clock.paused())
	{
		timeLine = "TIME PAUSED (P)";
	}
	else
	{
		const double daysPerSecond = clock.daysPerSecond();
		timeLine = daysPerSecond >= 1.0 ? "TIME " + fixed(daysPerSecond, 1) + " DAYS/S"
			: "TIME " + fixed(daysPerSecond * 24.0, 2) + " HOURS/S";
		timeLine += "  SPEED " + fixed(view.simulationSpeed, view.simulationSpeed < 1.0 ? 3 : 0) + "X";
		if (clock.encounterFactor() < 0.999)
			timeLine += "  ENCOUNTER SLOW-MOTION";
	}
	lines.push_back({ timeLine, kWhite });

	const CameraController::Mode mode = controller.mode();
	const char* modeName = "FOCUS";
	if (mode == CameraController::Mode::FreeFly)
		modeName = "FREE FLIGHT";
	else if (mode == CameraController::Mode::Chase)
		modeName = "CHASE VOYAGER";
	else if (mode == CameraController::Mode::Inspect)
		modeName = "INSPECT VOYAGER  (, . COMPONENT  I EXIT)";
	std::string cameraLine = std::string("CAMERA ") + modeName;
	if (mode == CameraController::Mode::FreeFly)
		cameraLine += "  (WHEEL SPEED X" + fixed(view.camera->speedMultiplier(), 2) + ")";
	const CelestialBody* focused = controller.focusedBody();
	if (mode == CameraController::Mode::Focus && focused != nullptr)
		cameraLine += ": " + focused->data().displayName;
	lines.push_back({ cameraLine, kWhite });

	if (view.voyager != nullptr)
	{
		const bool historical = view.voyager->flightMode() == Voyager2::FlightMode::Historical;
		lines.push_back({ std::string("FLIGHT ") + (historical ? "HISTORICAL (NASA/JPL HORIZONS)" : "MANUAL PILOT"),
			historical ? kWhite : kAccent });
		if (historical && ephemeris.hasVoyager())
		{
			const MissionEphemeris::Telemetry telemetry = ephemeris.voyagerTelemetry(julianDate);
			lines.push_back({ "VOYAGER " + fixed(telemetry.sunDistanceAu, 2) + " AU FROM SUN   " +
				fixed(telemetry.heliocentricSpeedKmPerSecond, 1) + " KM/S", kWhite });
			lines.push_back({ "NEAREST " + telemetry.nearestPlanet + " " +
				formatThousands(telemetry.nearestPlanetDistanceKm) + " KM", kWhite });
		}
		else if (!historical)
		{
			lines.push_back({ "SPEED " + fixed(glm::length(view.voyager->velocity()), 3) + " UNITS/S  (X BRAKES)", kWhite });
		}
	}
	if (mode == CameraController::Mode::Focus && focused != nullptr)
		lines.push_back({ focused->data().displayName + ": RADIUS " + formatThousands(focused->data().radiusKm) + " KM", kDim });
	for (const std::string& extra : view.extraLines)
		lines.push_back({ extra, kDim });
	lines.push_back({ "F1 CONTROLS", kDim });

	float panelWidth = 0.0f;
	for (const auto& entry : lines)
		panelWidth = std::max(panelWidth, TextRenderer::textWidth(entry.first, pixel));
	const float margin = pixel * 6.0f;
	m_panelRect = glm::vec4(margin - pixel * 4.0f, margin - pixel * 4.0f, margin + panelWidth + pixel * 4.0f,
		margin + line * static_cast<float>(lines.size()) + pixel * 1.0f);
	text.addRect(m_panelRect.x, m_panelRect.y, m_panelRect.z - m_panelRect.x, m_panelRect.w - m_panelRect.y,
		glm::vec4(0.0f, 0.02f, 0.05f, 0.55f));
	for (std::size_t i = 0; i < lines.size(); ++i)
		text.addText(margin, margin + line * static_cast<float>(i), lines[i].first, pixel, lines[i].second);
}

void HudOverlay::renderEncounterBanner(TextRenderer& text, const HudView& view, float pixel)
{
	const float width = static_cast<float>(view.width);
	const float height = static_cast<float>(view.height);
	const float margin = pixel * 6.0f;
	const double julianDate = view.mission->clock().julianDate();

	// Counts down to, then up from, the real closest approach.
	for (const MissionEphemeris::Encounter& encounter : view.mission->ephemeris().encounters())
	{
		const double hours = (julianDate - encounter.closestApproachJulianDate) * 24.0;
		if (std::abs(hours) > 72.0)
			continue;
		const int totalMinutes = static_cast<int>(std::abs(hours) * 60.0);
		std::ostringstream clock;
		clock << (hours < 0.0 ? "T-" : "T+") << std::setw(2) << std::setfill('0') << totalMinutes / 60
			<< ':' << std::setw(2) << totalMinutes % 60;
		const std::string title = upper(encounter.displayName) + " ENCOUNTER  " + clock.str();
		const std::string detail = "CLOSEST APPROACH " + formatThousands(encounter.closestApproachKm) + " KM FROM CENTRE";
		const float big = pixel * 1.5f;
		const float bannerY = height - margin - TextRenderer::lineHeight(big) - TextRenderer::lineHeight(pixel) * 2.0f;
		text.addShadowedText((width - TextRenderer::textWidth(title, big)) * 0.5f, bannerY, title, big, kAccent);
		text.addShadowedText((width - TextRenderer::textWidth(detail, pixel)) * 0.5f,
			bannerY + TextRenderer::lineHeight(big), detail, pixel, kWhite);
	}
}

void HudOverlay::renderHelp(TextRenderer& text, const HudView& view, float pixel)
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
		"  I INSPECT CLOSE-UP   , / . PREVIOUS / NEXT COMPONENT",
		"  W/S THRUST   A/D YAW   R/F PITCH   Q/E ROLL",
		"  SPACE/CTRL UP/DOWN   SHIFT BOOST   X BRAKE",
		"",
		"LIGHTING AND SHADING",
		"  K LIGHTING ON/OFF   F3 SHADING: FLAT, GOURAUD, PHONG, BLINN-PHONG, TOON",
		"  F4 RAY-TRACED SHADOWS: OFF, HARD, SOFT",
		"  F5 HEADLAMP SPOTLIGHT   F6 FILL DIRECTIONAL LIGHT   F7 SUN 1/D2 FALLOFF",
		"  F8 NORMAL AND SPECULAR MAPS",
		"  F9 RAY-TRACED VIEW   F10 RAY-TRACED REFLECTIONS",
		"",
		"DISPLAY",
		"  L LABELS   F2 HUD   F12 SCREENSHOT   ESC QUIT",
	};
	const float line = TextRenderer::lineHeight(pixel);
	float helpWidth = 0.0f;
	for (const std::string& entry : help)
		helpWidth = std::max(helpWidth, TextRenderer::textWidth(entry, pixel));
	const float helpHeight = line * static_cast<float>(help.size());
	const float x = (static_cast<float>(view.width) - helpWidth) * 0.5f;
	const float y = (static_cast<float>(view.height) - helpHeight) * 0.5f;
	text.addRect(x - pixel * 8.0f, y - pixel * 8.0f, helpWidth + pixel * 16.0f, helpHeight + pixel * 12.0f,
		glm::vec4(0.0f, 0.02f, 0.06f, 0.82f));
	for (std::size_t i = 0; i < help.size(); ++i)
		text.addText(x, y + line * static_cast<float>(i), help[i], pixel, i == 0 ? kAccent : kWhite);
}
