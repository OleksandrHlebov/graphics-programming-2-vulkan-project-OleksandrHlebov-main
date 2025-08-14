#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <array>
#include "Globals.h"

namespace HELP
{
	std::vector<char> ReadFile(const std::string& filename);

	void ErrorCallback(int error, const char* msg);

	void LoadScene();

	bool HasStencilComponent(VkFormat format);
}
