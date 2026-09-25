#ifndef SIMULATION_CLOCK_H
#define SIMULATION_CLOCK_H

#include <vector>

// Bible section 21: the single simulation date. Planets, Voyager's
// historical position and all telemetry read this Julian Date; nothing keeps
// its own copy. Real frame time (camera, input, manual piloting) never passes
// through here.
//
// Mission playback runs at kBaseDaysPerSecond x the user speed, except near
// a planetary closest approach, where the rate eases down smoothly to a few
// mission-hours per second. A flyby that really lasts hours is therefore
// watchable, while the 12-year cruise between planets still takes seconds.
class SimulationClock
{
public:
	void setRange(double startJulianDate, double endJulianDate);
	void setEncounters(std::vector<double> closestApproachJulianDates);

	void update(double realDeltaSeconds);

	void setJulianDate(double julianDate);
	double julianDate() const { return m_julianDate; }
	double startJulianDate() const { return m_start; }
	double endJulianDate() const { return m_end; }

	void setPaused(bool paused) { m_paused = paused; }
	bool paused() const { return m_paused; }
	void setSpeed(double speed) { m_speed = speed; }
	double speed() const { return m_speed; }
	void setEncounterSlowdown(bool enabled) { m_encounterSlowdown = enabled; }
	bool encounterSlowdown() const { return m_encounterSlowdown; }

	// Current mission days advanced per real second (0 while paused).
	double daysPerSecond() const;
	// 1 during cruise, falling toward kMinimumEncounterFactor at an encounter.
	double encounterFactor() const;

	static constexpr double kBaseDaysPerSecond = 120.0;

private:
	double m_start = 0.0;
	double m_end = 0.0;
	double m_julianDate = 0.0;
	double m_speed = 1.0;
	bool m_paused = false;
	bool m_encounterSlowdown = true;
	std::vector<double> m_encounters;

	// The rate is proportional to the time from closest approach inside this
	// window, which makes the approach an exponential ease-in (~4 s per side).
	static constexpr double kEncounterWindowDays = 60.0;
	static constexpr double kMinimumEncounterFactor = 0.0004; // ~1.2 mission-hours per second
};

#endif
