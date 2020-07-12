#include "PreCompiledHeader.hpp"

#include "Application.hpp"

#define GLFW_INCLUDE_VULKAN
#include "glfw3.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm/vec4.hpp"
#include "glm/glm/mat4x4.hpp"

class TestVec4 {
public:
	double x, y, z, w;
};




int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
	auto app = Application{};

	app.Start();

	while (app.IsRunning() == true){
		app.Update();
	}

	app.CleanUp();

	system("pause");
	return EXIT_SUCCESS;
}