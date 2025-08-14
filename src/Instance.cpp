#include "Instance.h"
#include <stdexcept>
#include <cstring>

void InstanceBuilder::Build(std::unique_ptr<Instance>& instance)
{
	VkApplicationInfo appInfo{};
	appInfo.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pEngineName			= m_EngineName.c_str();
	appInfo.engineVersion		= m_EngineVersion;
	appInfo.pApplicationName	= m_AppName.c_str();
	appInfo.applicationVersion	= m_AppVersion;
	appInfo.apiVersion			= apiVersion;

	if (ENABLE_VALIDATION_LAYERS && !CheckValidationLayerSupport())
		throw std::runtime_error("validation layers requested, but not available");

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(m_RequiredExtensions.size());
	createInfo.ppEnabledExtensionNames = m_RequiredExtensions.data();

	if (ENABLE_VALIDATION_LAYERS && m_DebugMsngrCreateInfo.has_value())
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
		createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&m_DebugMsngrCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}
	instance.reset(new Instance());

	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance->m_Instance);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create vkInstance");
}

bool InstanceBuilder::CheckValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : m_ValidationLayers)
	{
		bool layerFound{ false };

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
			return false;
	}

	return true;
}
