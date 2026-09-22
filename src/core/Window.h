#ifndef WINDOW_H
#define WINDOW_H

#include <string>

struct GLFWwindow;

// Owns the GLFW window and GL context lifetime. The only place in the program
// that talks to GLFW window/context APIs directly.
class Window
{
public:
	Window() = default;
	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	// Creates the window and a 3.3 core context, then loads GLAD.
	// Returns false and logs on failure; the caller should abort.
	bool create(int width, int height, const std::string& title);

	bool shouldClose() const;
	void requestClose();

	void pollEvents();
	void swapBuffers();
	void setTitle(const std::string& title);

	int width() const { return m_width; }
	int height() const { return m_height; }

	// 1.0 while the window is minimised, so projection never divides by zero.
	float aspectRatio() const;

	GLFWwindow* handle() const { return m_window; }

private:
	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
	static void errorCallback(int code, const char* description);

	GLFWwindow* m_window = nullptr;
	int m_width = 0;
	int m_height = 0;
	bool m_glfwInitialised = false;
};

#endif
