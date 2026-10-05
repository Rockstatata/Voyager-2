#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <functional>
#include <string>
#include <vector>

#include <glad/glad.h>

// Per-frame counters the Renderer fills in (docs/guide/11-performance.md).
struct RenderStats
{
	int drawCalls = 0;
	long long triangles = 0;
};

// `--benchmark <file>`: a fixed set of views, each settled then measured
// with vsync off. CPU time is the whole frame (update, render submission
// and swap); GPU time comes from a GL_TIME_ELAPSED query around render().
// The table is printed and written to <file> so changes can be compared.
class Benchmark
{
public:
	struct View
	{
		std::string name;
		std::function<void()> setup;
	};

	~Benchmark();

	void start(const std::string& outputPath, std::vector<View> views);
	bool active() const { return m_index < m_views.size(); }

	// Bracket Application::render(). Two alternating queries delay reads;
	// GL_QUERY_RESULT can still wait if the GPU has not finished that slot.
	void beginGpu();
	void endGpu();

	// Call once per frame after rendering. Returns false when finished.
	bool afterFrame(double deltaTime, const RenderStats& stats);

private:
	struct Result
	{
		std::string name;
		double cpuMs = 0.0;
		double gpuMs = 0.0;
		int drawCalls = 0;
		long long triangles = 0;
	};

	void report() const;

	std::string m_outputPath;
	std::vector<View> m_views;
	std::vector<Result> m_results;
	std::size_t m_index = 0;
	double m_settleRemaining = 0.0;
	double m_measured = 0.0;
	int m_frames = 0;
	int m_gpuFrames = 0;
	double m_cpuTotal = 0.0;
	double m_gpuTotal = 0.0;
	RenderStats m_lastStats;
	GLuint m_queries[2] = { 0, 0 };
	bool m_queryPending[2] = { false, false };
	int m_queryIndex = 0;

	static constexpr double kSettleSeconds = 2.5;
	static constexpr double kMeasureSeconds = 3.0;
};

#endif
