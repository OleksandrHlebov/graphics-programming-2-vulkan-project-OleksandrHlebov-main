#include "DebugMessenger.h"
#include <iostream>
#include <stdexcept>

DebugMessenger::DebugMessenger(VkInstance* instance) : m_InstancePtr{ instance }
{
	SetupDebugMessenger();
}

const VkDebugUtilsMessengerCreateInfoEXT* DebugMessenger::GetCreateInfo()
{
	static VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	if (createInfo.sType != VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = &DebugMessenger::DebugCallback;
		createInfo.pUserData = nullptr;
	}
	return &createInfo;
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessenger::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	if (messageSeverity >= m_MinSeverity)
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

VkResult DebugMessenger::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto function = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (function != nullptr)
		return function(instance, pCreateInfo, pAllocator, pDebugMessenger);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DebugMessenger::SetupDebugMessenger()
{
	if (CreateDebugUtilsMessengerEXT(*m_InstancePtr, GetCreateInfo(), nullptr, &m_Messenger) != VK_SUCCESS)
		throw std::runtime_error("failed to set up debug messenger");
}