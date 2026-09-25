#include "SimulationClock.h"

#include <algorithm>
#include <cmath>

void SimulationClock::setRange(double startJulianDate, double endJulianDate)
{
	m_start = startJulianDate;
	m_end = std::max(endJulianDate, startJulianDate);
	m_julianDate = std::clamp(m_julianDate, m_start, m_end);
}

void SimulationClock::setEncounters(std::vector<double> closestApproachJulianDates)
{
	m_encounters = std::move(closestApproachJulianDates);
}

double SimulationClock::encounterFactor() const
{
	if (!m_encounterSlowdown)
		return 1.0;

	double nearest = kEncounterWindowDays;
	for (double encounter : m_encounters)
		nearest = std::min(nearest, std::abs(m_julianDate - encounter));
	return std::clamp(nearest / kEncounterWindowDays, kMinimumEncounterFactor, 1.0);
}

double SimulationClock::daysPerSecond() const
{
	if (m_paused)
		return 0.0;
	return kBaseDaysPerSecond * m_speed * encounterFactor();
}

void SimulationClock::update(double realDeltaSeconds)
{
	if (m_paused || realDeltaSeconds <= 0.0)
		return;

	// Sub-stepping keeps a long frame from jumping across the slow zone of an
	// encounter: the rate is re-evaluated at least every 1/4 of the distance
	// to closest approach.
	double remaining = realDeltaSeconds;
	for (int step = 0; step < 64 && remaining > 0.0; ++step)
	{
		const double rate = daysPerSecond();
		double stepSeconds = remaining;
		if (m_encounterSlowdown)
		{
			double nearest = kEncounterWindowDays;
			for (double encounter : m_encounters)
				nearest = std::min(nearest, std::abs(m_julianDate - encounter));
			const double safeDays = std::max(nearest * 0.25, kEncounterWindowDays * kMinimumEncounterFactor);
			if (rate > 0.0)
				stepSeconds = std::min(stepSeconds, safeDays / rate);
		}
		m_julianDate += rate * stepSeconds;
		remaining -= stepSeconds;
	}
	m_julianDate = std::clamp(m_julianDate, m_start, m_end);
}

void SimulationClock::setJulianDate(double julianDate)
{
	m_julianDate = std::clamp(julianDate, m_start, m_end);
}
