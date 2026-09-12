#ifndef TIME_H
#define TIME_H

// Real frame time only (bible section 21, "real frame time" domain).
// Simulation time and its speed multiplier are a later phase and must never
// be folded into this value: camera and input always use unscaled deltaTime.
class Time
{
public:
	// Samples the clock once per frame. Pass glfwGetTime().
	void beginFrame(double now);

	double deltaTime() const { return m_deltaTime; }
	float deltaTimef() const { return (float)m_deltaTime; }
	double elapsed() const { return m_elapsed; }
	unsigned long long frameCount() const { return m_frameCount; }

	// Smoothed frames per second, useful for a HUD later.
	double fps() const;

private:
	// The first frame and any post-stall frame would otherwise produce a huge
	// delta and teleport the camera.
	static constexpr double kMaxDelta = 0.25;

	double m_startTime = 0.0;
	double m_lastTime = 0.0;
	double m_deltaTime = 0.0;
	double m_elapsed = 0.0;
	double m_smoothedDelta = 0.0;
	unsigned long long m_frameCount = 0;
	bool m_started = false;
};

#endif
