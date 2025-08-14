#include "Device.h"

#include <algorithm>

#include "TempHelpers.h"
#include "Globals.h"
#include "Instance.h"
#include "SwapChain.h"

#include <set>
#include <stdexcept>
#include <map>

VkFormat Device::FindSupportedFormats(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties properties{};
		vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &properties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
			return format;

		if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
			return format;
	}

	throw std::runtime_error("failed to find supported format");
}

uint32_t Device::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);
	for (uint32_t index{}; index < memProperties.memoryTypeCount; ++index)
	{
		if (typeFilter & (1 << index)
			&& (memProperties.memoryTypes[index].propertyFlags & properties) == properties)
		{
			return index;
		}
	}
	throw std::runtime_error("failed to find suitable memory type");
}

Device::QueueFamilyIndices Device::FindQueueFamilies(VkSurfaceKHR surface)
{
	return FindQueueFamilies(m_PhysicalDevice, surface);
}

Device::QueueFamilyIndices Device::FindQueueFamilies(VkPhysicalDevice physDevice, VkSurfaceKHR surface)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount{};
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, queueFamilies.data());


	int index{};
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphicsFamily = index;

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, index, surface, &presentSupport);

		if (presentSupport)
			indices.presentFamily = index;

		if (indices.IsComplete())
			break;

		++index;
	}

	return indices;
}

void Device::SetObjectName(VkObjectType type, uint64_t handle, const char* name)
{
	VkDebugUtilsObjectNameInfoEXT info{};
	info.sType			= VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	info.objectType		= type;
	info.objectHandle	= handle;
	info.pObjectName	= name;

	#ifndef NDEBUG
	vkSetDebugUtilsObjectNameEXT(m_Device, &info);
	#endif
}

void Device::Destroy()
{
	vkDestroyDevice(m_Device, nullptr);
}

void DeviceBuilder::Build(std::unique_ptr<Device>& device, Instance* instance, VkSurfaceKHR surface)
{
	device.reset(new Device());

	PickPhysicalDevice(&device->m_PhysicalDevice, *instance->GetInstancePtr(), surface);

	Device::QueueFamilyIndices indices = device->FindQueueFamilies(surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies{ indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority{ 1.f };
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	m_CustomBorderColorFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT;
	m_DeviceFeatures13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	m_DeviceFeatures13.pNext = &m_CustomBorderColorFeatures;
	m_DeviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	m_DeviceFeatures12.pNext = &m_DeviceFeatures13;
	m_DeviceFeatures11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	m_DeviceFeatures11.pNext = &m_DeviceFeatures12;
	m_DeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	m_DeviceFeatures.pNext = &m_DeviceFeatures11;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext = &m_DeviceFeatures;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pEnabledFeatures = nullptr;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(m_Extensions.size());
	createInfo.ppEnabledExtensionNames = m_Extensions.data();

	if (ENABLE_VALIDATION_LAYERS)
	{
		const std::vector<const char*>& validationLayers = instance->GetValidationLayers();
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
		createInfo.enabledLayerCount = 0;

	if (vkCreateDevice(device->m_PhysicalDevice, &createInfo, nullptr, &device->m_Device) != VK_SUCCESS)
		throw std::runtime_error("failed to create logical device");

	vkGetDeviceQueue(device->m_Device, indices.graphicsFamily.value(), 0, &device->m_GraphicsQueue);
	vkGetDeviceQueue(device->m_Device, indices.presentFamily.value(), 0, &device->m_PresentQueue);

	#ifndef NDEBUG
	device->vkSetDebugUtilsObjectNameEXT	= (PFN_vkSetDebugUtilsObjectNameEXT)	vkGetDeviceProcAddr(device->m_Device, "vkSetDebugUtilsObjectNameEXT");
	#endif
}

DeviceBuilder& DeviceBuilder::SetEnabledFeatures(const VkPhysicalDeviceVulkan13Features& features)
{
	m_DeviceFeatures13 = features;
	return *this;
}

DeviceBuilder& DeviceBuilder::SetEnabledFeatures(const VkPhysicalDeviceVulkan12Features& features)
{
	m_DeviceFeatures12 = features;
	return *this;
}

DeviceBuilder& DeviceBuilder::SetEnabledFeatures(const VkPhysicalDeviceVulkan11Features& features)
{
	m_DeviceFeatures11 = features;
	return *this;
}

DeviceBuilder& DeviceBuilder::SetEnabledFeatures(const VkPhysicalDeviceFeatures2& features)
{
	m_DeviceFeatures = features;
	return *this;
}

DeviceBuilder& DeviceBuilder::SetEnabledFeatures(const VkPhysicalDeviceCustomBorderColorFeaturesEXT& features)
{
	m_CustomBorderColorFeatures = features;
	return *this;
}

DeviceBuilder& DeviceBuilder::SetEnabledFeatures(const VkPhysicalDeviceFeatures& features)
{
	m_DeviceFeatures.features = features;
	return *this;
}

void DeviceBuilder::PickPhysicalDevice(VkPhysicalDevice* physDevice, VkInstance instance, VkSurfaceKHR surface)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0)
		throw std::runtime_error("failed to find GPUs with Vulkan support");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	std::multimap<int, VkPhysicalDevice> candidates;

	for (const auto& device : devices)
	{
		int score = RateDeviceSuitability(device, surface);
		candidates.insert(std::make_pair(score, device));
	}

	auto bestCandidate = std::ranges::max_element(candidates);

	if (bestCandidate->first > 0)
		*physDevice = bestCandidate->second;
	else
		throw std::runtime_error("failed to find a suitable GPU");
}

int DeviceBuilder::RateDeviceSuitability(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	int score{};

	VkPhysicalDeviceProperties deviceProperties = {};

	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && m_PreferdGPU)
		score += 10000;

	Device::QueueFamilyIndices indices{ Device::FindQueueFamilies(device, surface) };

	bool extensionsSupported{ CheckDeviceExtensionSupport(device) };
	bool swapChainAdequate{};
	if (extensionsSupported)
	{
		Swapchain::SupportDetails swapChainSupport{ Swapchain::QuerySwapchainSupport(device, surface) };
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	if (indices.graphicsFamily == indices.presentFamily)
		score += 100;

	score += deviceProperties.limits.maxImageDimension2D;

	const bool areAllFeaturesAvailable{ 
		CheckFeatureSupport(device)
	};

	if (!areAllFeaturesAvailable || !extensionsSupported || !swapChainAdequate || !indices.IsComplete())
		return 0;

	return score;
}

bool DeviceBuilder::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(m_Extensions.begin(), m_Extensions.end());

	for (const auto& extension : availableExtensions)
		requiredExtensions.erase(extension.extensionName);

	return requiredExtensions.empty();
}

bool DeviceBuilder::CheckFeatureSupport(VkPhysicalDevice device)
{
	VkPhysicalDeviceCustomBorderColorFeaturesEXT customBorderColorFeatures{};
	customBorderColorFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT;
	VkPhysicalDeviceVulkan13Features deviceFeatures13{};
	deviceFeatures13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	deviceFeatures13.pNext = &customBorderColorFeatures;
	VkPhysicalDeviceVulkan12Features deviceFeatures12{};
	deviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	deviceFeatures12.pNext = &deviceFeatures13;
	VkPhysicalDeviceVulkan11Features deviceFeatures11{};
	deviceFeatures11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	deviceFeatures11.pNext = &deviceFeatures12;
	VkPhysicalDeviceFeatures2 deviceFeatures{};
	deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures.pNext = &deviceFeatures11;

	vkGetPhysicalDeviceFeatures2(device, &deviceFeatures);

	bool hasAllFeatures = 
		CompareFeatures<VkPhysicalDeviceCustomBorderColorFeaturesEXT>(&customBorderColorFeatures, &m_CustomBorderColorFeatures) &&
		CompareFeatures<VkPhysicalDeviceVulkan13Features>(&deviceFeatures13, &m_DeviceFeatures13) &&
		CompareFeatures<VkPhysicalDeviceVulkan12Features>(&deviceFeatures12, &m_DeviceFeatures12) &&
		CompareFeatures<VkPhysicalDeviceVulkan11Features>(&deviceFeatures11, &m_DeviceFeatures11) &&
		CompareFeatures<VkPhysicalDeviceFeatures2>(&deviceFeatures, &deviceFeatures);

	//const VkBool32* enabledCBCFeatures{  };

	return hasAllFeatures;
}
