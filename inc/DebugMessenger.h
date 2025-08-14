#pragma once
#include <vulkan/vulkan.h>
#include <memory>

class Instance;
class DebugMessenger final
{
public:
	DebugMessenger(VkInstance* instance);
	~DebugMessenger() = default;
	
	DebugMessenger(const DebugMessenger&) 				= delete;
	DebugMessenger(DebugMessenger&&) noexcept 			= delete;
	DebugMessenger& operator=(const DebugMessenger&) 	 	= delete;
	DebugMessenger& operator=(DebugMessenger&&) noexcept 	= delete;

	VkDebugUtilsMessengerEXT* GetMessengerPtr() { return &m_Messenger; };

	static void SetMinMessageSeverity(VkDebugUtilsMessageSeverityFlagBitsEXT severity) { m_MinSeverity = severity; }
	static const VkDebugUtilsMessengerCreateInfoEXT* GetCreateInfo();

private:
	inline static VkDebugUtilsMessageSeverityFlagBitsEXT m_MinSeverity{ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT };

	VkDebugUtilsMessengerEXT m_Messenger;

	VkInstance* m_InstancePtr;

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	static VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);

	void SetupDebugMessenger();

	static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
};