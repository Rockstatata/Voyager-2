#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>
#include <string>

#include "Input.h"
#include "Time.h"
#include "Window.h"
#include "../rendering/Camera.h"
#include "../rendering/Renderer.h"
#include "../scene/Scene.h"

class Shader;
class Mesh;

// Top-level owner of the window, subsystems and loop (bible section 10).
// main() does nothing but construct this and call run().
//
// The loop keeps update and render strictly separate: update() may change
// scene state, render() may not.
class Application
{
public:
	Application();
	~Application();

	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	// Creates the window/context and loads GPU resources.
	// Returns false if the window, GLAD or the shader failed.
	bool initialize(int width = 800, int height = 800,
					const std::string& title = "Voyager 2 Explorer");

	void run();

private:
	bool loadShaders();
	void buildScene();
	void update(double deltaTime);
	void render();

	Window m_window;
	Input m_input;
	Time m_time;
	Camera m_camera;
	Renderer m_renderer;
	Scene m_scene;

	std::unique_ptr<Shader> m_shader;

	// One quad uploaded once and shared by both test objects; proves the
	// shared-geometry path before the sphere phase relies on it.
	std::shared_ptr<Mesh> m_quadMesh;

	SceneObject* m_firstQuad = nullptr;  // non-owning; the Scene owns them
	SceneObject* m_secondQuad = nullptr;

	bool m_initialized = false;
};

#endif
