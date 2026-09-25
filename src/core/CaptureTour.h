#ifndef CAPTURE_TOUR_H
#define CAPTURE_TOUR_H

#include <functional>
#include <string>
#include <vector>

// Scripted screenshot sequence (`--capture`, `--capture-bodies`): each shot
// sets the scene up, waits a real-time settle period for camera fly-to
// transitions, then saves the back buffer. Used for documentation images and
// visual verification.
class CaptureTour
{
public:
	struct Shot
	{
		std::string fileName;
		double settleSeconds = 2.5;
		std::function<void()> setup;
	};

	void start(const std::string& directory, std::vector<Shot> shots);
	bool active() const { return m_index < m_shots.size(); }

	// Call once per rendered frame, before the buffer swap. Saves the current
	// shot when it has settled and starts the next. Returns false once the
	// tour has finished (the caller then closes the window).
	bool afterRender(double deltaTime, int width, int height);

private:
	std::string m_directory;
	std::vector<Shot> m_shots;
	std::size_t m_index = 0;
	double m_secondsRemaining = 0.0;
};

// Uncompressed 24-bit BMP of the current GL back buffer.
bool saveScreenshot(const std::string& path, int width, int height);

#endif
