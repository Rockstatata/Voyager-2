#include "src/core/Application.h"

// Entry point only. Everything else belongs to Application
// (bible section 51, failure mode F1: "everything in main.cpp").
int main(int argc, char** argv)
{
	Application app;

	if (!app.initialize(argc, argv))
		return -1;

	app.run();
	return 0;
}
