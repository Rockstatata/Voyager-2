#include "Benchmark.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

Benchmark::~Benchmark()
{
	if (m_queries[0] != 0)
		glDeleteQueries(2, m_queries);
}

void Benchmark::start(const std::string& outputPath, std::vector<View> views)
{
	m_outputPath = outputPath;
	m_views = std::move(views);
	m_results.clear();
	m_index = 0;
	if (m_queries[0] == 0)
		glGenQueries(2, m_queries);
	if (!m_views.empty())
	{
		m_views[0].setup();
		m_settleRemaining = kSettleSeconds;
	}
	std::cout << "[APP] benchmark: " << m_views.size() << " views, vsync off" << std::endl;
}

void Benchmark::beginGpu()
{
	if (!active())
		return;
	// Collect the query issued on the previous use of this slot.
	GLuint& query = m_queries[m_queryIndex];
	if (m_queryPending[m_queryIndex])
	{
		GLuint64 nanoseconds = 0;
		glGetQueryObjectui64v(query, GL_QUERY_RESULT, &nanoseconds);
		m_queryPending[m_queryIndex] = false;
		if (m_settleRemaining <= 0.0)
		{
			m_gpuTotal += static_cast<double>(nanoseconds) * 1e-6;
			++m_gpuFrames;
		}
	}
	glBeginQuery(GL_TIME_ELAPSED, query);
}

void Benchmark::endGpu()
{
	if (!active())
		return;
	glEndQuery(GL_TIME_ELAPSED);
	m_queryPending[m_queryIndex] = true;
	m_queryIndex = 1 - m_queryIndex;
}

bool Benchmark::afterFrame(double deltaTime, const RenderStats& stats)
{
	if (!active())
		return false;

	if (m_settleRemaining > 0.0)
	{
		m_settleRemaining -= deltaTime;
		if (m_settleRemaining <= 0.0)
		{
			m_measured = 0.0;
			m_frames = 0;
			m_gpuFrames = 0;
			m_cpuTotal = 0.0;
			m_gpuTotal = 0.0;
		}
		return true;
	}

	m_measured += deltaTime;
	m_cpuTotal += deltaTime * 1000.0;
	++m_frames;
	m_lastStats = stats;
	if (m_measured < kMeasureSeconds)
		return true;

	Result result;
	result.name = m_views[m_index].name;
	result.cpuMs = m_cpuTotal / m_frames;
	result.gpuMs = m_gpuFrames > 0 ? m_gpuTotal / m_gpuFrames : 0.0;
	result.drawCalls = m_lastStats.drawCalls;
	result.triangles = m_lastStats.triangles;
	m_results.push_back(result);
	std::cout << "[APP] benchmark " << result.name << ": " << std::fixed << std::setprecision(2)
			  << result.cpuMs << " ms frame, " << result.gpuMs << " ms GPU" << std::endl;

	++m_index;
	m_queryPending[0] = m_queryPending[1] = false;
	if (!active())
	{
		report();
		return false;
	}
	m_views[m_index].setup();
	m_settleRemaining = kSettleSeconds;
	return true;
}

void Benchmark::report() const
{
	std::ostringstream table;
	table << std::left << std::setw(26) << "view" << std::right << std::setw(10) << "frame ms"
		  << std::setw(8) << "fps" << std::setw(10) << "GPU ms" << std::setw(8) << "draws"
		  << std::setw(12) << "triangles" << "\n";
	double totalCpu = 0.0;
	for (const Result& result : m_results)
	{
		totalCpu += result.cpuMs;
		table << std::left << std::setw(26) << result.name << std::right << std::fixed << std::setprecision(2)
			  << std::setw(10) << result.cpuMs << std::setw(8) << std::setprecision(0) << 1000.0 / result.cpuMs
			  << std::setw(10) << std::setprecision(2) << result.gpuMs << std::setw(8) << result.drawCalls
			  << std::setw(12) << result.triangles << "\n";
	}
	if (!m_results.empty())
		table << std::left << std::setw(26) << "mean" << std::right << std::fixed << std::setprecision(2)
			  << std::setw(10) << totalCpu / m_results.size() << "\n";
	std::cout << table.str();
	std::ofstream file(m_outputPath);
	if (file)
	{
		file << table.str();
		std::cout << "[APP] benchmark written to " << m_outputPath << std::endl;
	}
}
