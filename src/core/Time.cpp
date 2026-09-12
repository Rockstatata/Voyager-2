#include "Time.h"

void Time::beginFrame(double now)
{
	if (!m_started)
	{
		m_started = true;
		m_startTime = now;
		m_lastTime = now;
		m_deltaTime = 0.0;
		m_elapsed = 0.0;
		m_frameCount = 1;
		return;
	}

	double delta = now - m_lastTime;
	if (delta < 0.0)
		delta = 0.0;
	if (delta > kMaxDelta)
		delta = kMaxDelta;

	m_lastTime = now;
	m_deltaTime = delta;
	m_elapsed = now - m_startTime;
	++m_frameCount;

	// Exponential moving average; steadier than the raw delta for display.
	m_smoothedDelta = (m_smoothedDelta == 0.0) ? delta : (m_smoothedDelta * 0.9 + delta * 0.1);
}

double Time::fps() const
{
	if (m_smoothedDelta <= 0.0)
		return 0.0;
	return 1.0 / m_smoothedDelta;
}
