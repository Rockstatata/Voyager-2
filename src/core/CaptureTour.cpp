#include "CaptureTour.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>

#include <glad/glad.h>

void CaptureTour::start(const std::string& directory, std::vector<Shot> shots)
{
	m_directory = directory;
	m_shots = std::move(shots);
	m_index = 0;
	std::filesystem::create_directories(directory);
	if (m_shots.empty())
		return;
	m_shots[0].setup();
	m_secondsRemaining = m_shots[0].settleSeconds;
	std::cout << "[APP] capture tour: " << m_shots.size() << " shots -> " << directory << std::endl;
}

bool CaptureTour::afterRender(double deltaTime, int width, int height)
{
	if (!active())
		return true;
	m_secondsRemaining -= deltaTime;
	if (m_secondsRemaining > 0.0)
		return true;

	const std::string path = m_directory + "/" + m_shots[m_index].fileName;
	if (saveScreenshot(path, width, height))
		std::cout << "[APP] capture saved: " << path << std::endl;
	if (++m_index >= m_shots.size())
		return false;
	m_shots[m_index].setup();
	m_secondsRemaining = m_shots[m_index].settleSeconds;
	return true;
}

bool saveScreenshot(const std::string& path, int width, int height)
{
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

	// OpenGL's bottom-up rows are BMP's native order.
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
