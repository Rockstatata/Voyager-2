#include <iostream>
#include <memory>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shaderClass.h"
#include "VBO.h"
#include "EBO.h"
#include "VAO.h"

#include "src/scene/Scene.h"
#include "src/rendering/Renderer.h"

using namespace std;
using namespace glm;

const unsigned int width = 800;
const unsigned int height = 800;

int main() {
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//GLfloat vertices[] = {
	//	-0.5f,		-0.5f * sqrt(3) / 3,										+0.0f, 0.1019f, 0.7372f, 0.6117f, // Lower left, 0
	//	+0.5f,		-0.5f * sqrt(3) / 3,										+0.0f, 0.1803f, 0.8000f, 0.4431f, // Lower right, 1
	//	+0.0f,		+0.5f * sqrt(3) * 2 / 3,									+0.0f, 0.2039f, 0.5960f, 0.8588f, // Upper, 2
	//	-0.5f / 2,	((+0.5f * sqrt(3) * 2 / 3) + (-0.5f * sqrt(3) / 3)) / 2,	+0.0f, 0.6078f, 0.3490f, 0.7137f, // Inner left, 3
	//	+0.5f / 2,	((+0.5f * sqrt(3) * 2 / 3) + (-0.5f * sqrt(3) / 3)) / 2,	+0.0f, 0.2039f, 0.2862f, 0.3686f, // Inner right, 4
	//	+0.0f,		-0.5f * sqrt(3) / 3,										+0.0f, 0.9450f, 0.7686f, 0.0588f  // Inner down, 5
	//};

	//GLuint indices[] = {
	//	3, 0, 5,
	//	4, 5, 1,
	//	2, 3, 4
	//};

	//GLfloat vertices[] = {
	//	// Position                 // Color
	//	// x      y      z          r       g       b

	//	-0.5f, -0.5f,  0.5f,       1.0f,   0.0f,   0.0f,   // 0: Front-left
	//	 0.5f, -0.5f,  0.5f,       0.0f,   1.0f,   0.0f,   // 1: Front-right
	//	 0.5f, -0.5f, -0.5f,       0.0f,   0.0f,   1.0f,   // 2: Back-right
	//	-0.5f, -0.5f, -0.5f,       1.0f,   1.0f,   0.0f,   // 3: Back-left

	//	 0.0f,  0.5f,  0.0f,       1.0f,   0.0f,   1.0f    // 4: Top
	//};

	//GLuint indices[] = {
	//	// Four triangular sides
	//	0, 1, 4,      // Front
	//	1, 2, 4,      // Right
	//	2, 3, 4,      // Back
	//	3, 0, 4,      // Left

	//	// Square base (two triangles)
	//	0, 3, 2,
	//	0, 2, 1
	//};

	GLfloat vertices[] = {
		0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, //0
		1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, //1
		1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, //2
		0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f // 3
	};

	GLuint indices[] = {
		0, 1, 2,
		0, 3, 2
	};

	GLFWwindow* window = glfwCreateWindow(width, height, "Window 1", NULL, NULL);
	if (window == NULL) {
		cout << "Failed to create window!" << endl;
		return -1;
	}
	glfwMakeContextCurrent(window);

	gladLoadGL();

	glViewport(0, 0, width, height);

	Shader shaderProgram("default.vert", "default.frag");

	// The cube (currently a flat quad) becomes one object in the reusable scene.
	auto cube = std::make_unique<SceneObject>();
	cube->setMesh(std::make_unique<Mesh>(vertices, sizeof(vertices),
										 indices, sizeof(indices),
										 (GLsizei)(sizeof(indices) / sizeof(GLuint))));

	Scene scene;
	SceneObject& cubeObj = scene.addObject(std::move(cube));

	Renderer renderer;
	renderer.setShader(&shaderProgram);
	// Static camera, matching the starter's hardcoded view/projection.
	renderer.setViewProjection(
		translate(mat4(1.0f), vec3(0.0f, 0.0f, -2.0f)),
		perspective(radians(45.0f), float(width) / float(height), 0.1f, 100.0f));

	float rotation = 0.0f;
	double prevTime = glfwGetTime();

	while (glfwWindowShouldClose(window) == false) {
		double currTime = glfwGetTime();
		double dt = currTime - prevTime;
		if (currTime - prevTime >= 1.0 / 60.0) {
			//rotation += 0.1f;
			prevTime = currTime;
		}

		// Preserve the starter's original model transform.
		Transform& t = cubeObj.transform();
		t.rotation = angleAxis((double)radians(rotation), dvec3(0.0, 1.0, 0.0))
				   * angleAxis(-45.0, dvec3(0.0, 0.0, 1.0));
		t.position = dvec3(-0.5, -0.5, 0.0);

		scene.update(dt);

		renderer.beginFrame();
		scene.render(renderer);
		renderer.endFrame();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	shaderProgram.Delete();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
