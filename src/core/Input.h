#ifndef INPUT_H
#define INPUT_H

#include <array>
#include <glm/glm.hpp>

struct GLFWwindow;

// Central input state (bible section 36). Every system asks Input; nothing
// else calls glfwGetKey directly, so rebinding stays a one-file change.
//
// Distinguishes continuous from edge-triggered queries:
//   keyDown     - held this frame        (thrust, steering)
//   keyPressed  - went down this frame   (mode switch, pause, toggles)
//   keyReleased - came up this frame
class Input
{
public:
	void attach(GLFWwindow* window);

	// Call once per frame, before anything reads input.
	void update();

	bool keyDown(int key) const;
	bool keyPressed(int key) const;
	bool keyReleased(int key) const;

	bool mouseButtonDown(int button) const;
	bool mouseButtonPressed(int button) const;

	glm::dvec2 mousePosition() const { return m_mousePosition; }
	glm::dvec2 mouseDelta() const { return m_mouseDelta; }
	// Mouse-wheel notches since the previous update(); positive = away from user.
	double scrollDelta() const { return m_scrollDelta; }

	// Cursor capture, for mouse-look. Re-centring on capture avoids the jump
	// a stale cursor position would otherwise cause on the first frame.
	void setCursorCaptured(bool captured);
	bool cursorCaptured() const { return m_cursorCaptured; }

private:
	static constexpr int kKeyCount = 512;     // GLFW_KEY_LAST is 348
	static constexpr int kMouseButtonCount = 8;

	// GLFW reports the wheel only through a callback; it accumulates here
	// and is consumed once per update(). One window, so one accumulator.
	static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);
	static inline double s_pendingScroll = 0.0;

	static bool validKey(int key) { return key >= 0 && key < kKeyCount; }
	static bool validButton(int button) { return button >= 0 && button < kMouseButtonCount; }

	GLFWwindow* m_window = nullptr;           // non-owning; Window outlives Input

	std::array<bool, kKeyCount> m_keys{};
	std::array<bool, kKeyCount> m_previousKeys{};
	std::array<bool, kMouseButtonCount> m_mouseButtons{};
	std::array<bool, kMouseButtonCount> m_previousMouseButtons{};

	glm::dvec2 m_mousePosition{ 0.0 };
	glm::dvec2 m_mouseDelta{ 0.0 };
	double m_scrollDelta = 0.0;
	bool m_cursorCaptured = false;
	bool m_hasMouseSample = false;
};

#endif
