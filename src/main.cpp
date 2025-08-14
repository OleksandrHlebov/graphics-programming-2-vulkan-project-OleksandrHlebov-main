// camera movement while right mouse button is held down:
// WASD  -> horizontal movement
// E, Q  -> up, down
// SHIFT -> double speed

#include <iostream>
#include "DynamicRenderingApp.h"

// app that makes use of dynamic rendering
using CurrentApp = DynamicRenderingApp;

int main()
{
	CurrentApp app{};

	try
	{
		app.Run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
